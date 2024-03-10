#include <vector>

#pragma warning(disable : 4996)

using namespace std;

int lzss_decode(char *filename);
int lz10_decode(char *filename);
int test_decode(char *filename);

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage:\nyyt2_decode <compressed_file>\n");
		return 0;
	}
	unsigned int head = 0;
	FILE *file = fopen(argv[1], "rb");
	fseek(file, 0, 2);
	if (ftell(file) > 4)
	{
		rewind(file);
		fread(&head, 4, 1, file);
	}
	else return -1;
	fclose(file);
	if (head == 0x4C5A4F31)
	{
		printf("Decoding LZ10: %s ... ", argv[1]);
		if (!lz10_decode(argv[1])) printf("Done!\n"); else printf("Fail!\n");
	}
	else if ((head & 0xFF) == 0x10)
	{
		printf("Decoding LZSS: %s ... ", argv[1]);
		if (!lzss_decode(argv[1])) printf("Done!\n"); else printf("Fail!\n");
	}
	else return -1;
	return 0;
}

int lz10_decode(char *filename)
{
	unsigned int dsize = 0, fsize = 0, ca = 0, cb = 0, cc = 0, sp_flag = 0, pos = 0, len = 0;
	FILE *file = fopen(filename, "rb");
	fread(&dsize, 4, 1, file);
	fread(&dsize, 4, 1, file);
	fread(&fsize, 4, 1, file);
	fsize += 0x0C;
	vector<unsigned char> data;
	ca = getc(file);
	if (ca >= 15) sp_flag = 1;
	if (ca <= 0x11) 
	{
		fseek(file, -1, 1);
		sp_flag = 2;
		ca = 0;
	}
	else ca -= 0x11;
	while (ca--) data.push_back(getc(file));
	while ((unsigned int)ftell(file) < fsize && data.size() < dsize)
	{
		ca = getc(file);
		if (ca >= 0x10) sp_flag = 0;
		if (ca >= 0x40)
		{
			cb = getc(file);
			pos = (ca >> 2 & 0x07) + (cb << 3) + 0x01;
			len = (ca >> 5) + 1;
			if (data.size() < pos) break;
			while (len--) data.push_back(data[data.size() - pos]);
			ca = ca & 0x03;
		}
		else if (ca >= 0x10)
		{
			len = ca >= 0x20 ? (ca & 0x1F) + 2 : (ca & 0x07) + 2;
			pos = ca >= 0x20 ? 0x01 : ((ca & 0x08) << 0x0B) + 0x4000;
			if (len == 2)
			{
				while ((cb = getc(file)) == 0) len += 0xFF;
				len += ca >= 0x20 ? cb + 0x1F : cb + 0x07;
			}
			ca = getc(file);
			cb = getc(file);
			pos += (cb << 6) + (ca >> 2);
			if (data.size() < pos) break;
			while (len--) data.push_back(data[data.size() - pos]);
			ca = ca & 0x03;
		}
		else if (sp_flag == 2)
		{
			sp_flag = 1;
			if (ca == 0)
			{
				while ((cb = getc(file)) == 0) ca += 0xFF;
				ca += cb + 0x12;
			}
			else ca += 0x03;
		}
		else
		{
			cb = getc(file);
			pos = (ca >> 2) + (cb << 2) + 0x01;
			len = 0x02;
			if (sp_flag == 1)
			{
				pos += 0x800;
				len += 0x01;
				sp_flag = 0;
			}
			if (data.size() < pos) break;
			while (len--) data.push_back(data[data.size() - pos]);
			ca = ca & 0x03;
		}
		if (ca == 0) sp_flag = 2;
		while (ca--) data.push_back(getc(file));
	}
	fclose(file);
	if (data.size() != dsize) return -1;
	file = fopen(filename, "wb");
	for (auto i : data) putc(i, file);
	return 0;
}

int lzss_decode(char *filename)
{
	unsigned int dsize = 0, fsize = 0, flags = 0, mask = 0, pos = 0, len = 0;
	FILE *file = fopen(filename, "rb");
	fseek(file, 0, 2);
	fsize = ftell(file);
	rewind(file);
	fread(&dsize, 4, 1, file);
	dsize >>= 8;
	vector<unsigned char> data;
	while ((unsigned int)ftell(file) < fsize && data.size() < dsize)
	{
		if ((mask >>= 1) == 0)
		{
			flags = getc(file);
			mask = 0x80;
		}
		if (flags & mask)
		{
			pos = getc(file);
			pos = (pos << 8) | getc(file);
			len = (pos >> 12) + 3;
			pos = (pos & 0xFFF) + 1;
			if (data.size() < pos) break;
			while (len--) data.push_back(data[data.size() - pos]);
		}
		else data.push_back(getc(file));
	}
	fclose(file);
	if (data.size() != dsize) return -1;
	file = fopen(filename, "wb");
	for (auto i : data) putc(i, file);
	return 0;
}

