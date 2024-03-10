#include <imgConvLib.h>

#pragma warning(disable : 4996)

int imgOut(char *imgFileName, char *ntftFileName, char *ntfpFileName, unsigned int width);
int imgIn(char *imgFileName, char *ntftFileName, char *ntfpFileName, unsigned int width);

int main(int argc, char* argv[])
{
	if (argc < 6)
	{
		printf("usage:\n");
		printf("yyt2_ntft out <imgae_file> <ntft_file> <ntfp_file> <width>\n");
		printf("yyt2_ntft in <imgae_file> <ntft_file> <ntfp_file> <width>\n");
	}
	else if (!strcmp("out", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		if (!imgOut(argv[2], argv[3], argv[4], atoi(argv[5]))) printf("Done!\n"); else printf("Fail!\n");
	}
	else if (!strcmp("in", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		if (!imgIn(argv[2], argv[3], argv[4], atoi(argv[5]))) printf("Done!\n"); else printf("Fail!\n");
	}
	return 0;
}

int imgOut(char *imgFileName, char *ntftFileName, char *ntfpFileName, unsigned int width)
{
	FILE *pal = fopen(ntfpFileName, "rb");
	if (pal == NULL) return -1;
	fseek(pal, 0, 2);
	vector<unsigned char> palData(ftell(pal));
	rewind(pal);
	for (size_t i = 0; i < palData.size(); i++) palData[i] = getc(pal);
	fclose(pal);
	vector<vector<Color>> palList = ByteToPal(palData, RGB555, palData.size()/2, 1, false, true);
	vector<unsigned char>().swap(palData);
	FILE *index = fopen(ntftFileName, "rb");
	if (index == NULL) return -1;
	fseek(index, 0, 2);
	vector<unsigned char> indexData(ftell(index));
	rewind(index);
	for (size_t i = 0; i < indexData.size(); i++) indexData[i] = getc(index);
	fclose(index);
	vector<unsigned char> indexList;
	for (auto i : indexData)
	{
		if (palList[0].size() == 0x100) indexList.push_back(i);
		else if (palList[0].size() == 0x10)
		{
			indexList.push_back(i & 0xF);
			indexList.push_back((i >> 4) & 0xF);
		}
		else
		{
			indexList.push_back(i & 0x3);
			indexList.push_back((i >> 2) & 0x3);
			indexList.push_back((i >> 4) & 0x3);
			indexList.push_back((i >> 6) & 0x3);
		}
	}
	vector<unsigned char>().swap(indexData);
	unsigned int height = indexList.size() / width;

	ImageData img(width, height);
	for (unsigned int i = 0; i < indexList.size(); i++) img.set(palList[0][indexList[i]], i);
	img.write(imgFileName);
	img.release();
	vector<unsigned char>().swap(indexList);
	for (size_t i = 0; i < palList.size(); i++) vector<Color>().swap(palList[i]);
	vector<vector<Color>>().swap(palList);
	return 0;
}

int imgIn(char *imgFileName, char *ntftFileName, char *ntfpFileName, unsigned int width)
{
	FILE *pal = fopen(ntfpFileName, "rb");
	if (pal == NULL) return -1;
	fseek(pal, 0, 2);
	vector<unsigned char> palData(ftell(pal));
	rewind(pal);
	for (size_t i = 0; i < palData.size(); i++) palData[i] = getc(pal);
	fclose(pal);
	vector<vector<Color>> palList = ByteToPal(palData, RGB555, palData.size() / 2, 1, false, true);
	vector<unsigned char>().swap(palData);
	if (palList[0].size() != 0x04 && palList[0].size() != 0x10 && palList[0].size() != 0x100) return -1;

	ImageData img(imgFileName);
	unsigned char pal_mode = palList[0].size() == 0x100 ? 1 : 0;
	vector<unsigned char> indexList;
	unsigned int colNum = palList[0].size();
	ImageToIP(img, indexList, palList[0], palList[0][0], pal_mode, colNum);
	while (palList[0].size() < colNum) palList[0].push_back({ 0, 0, 0, 0 });

	FILE *pal_new = fopen(ntfpFileName, "wb");
	palData = PalToByte(palList, RGB555, false, true);
	for (auto i : palData) putc(i, pal_new);
	fclose(pal_new);
	FILE *index = fopen(ntftFileName, "wb");
	if (palList[0].size() == 0x100 || palList[0].size() == 0x10) for (auto i : indexList) putc(i, index);
	else for (unsigned int i = 0; i < indexList.size(); i += 2) putc((indexList[i + 1] & 0x03) << 4 | ((indexList[i + 1] >> 4) & 0x03) << 6 | ((indexList[i] >> 4) & 0x03) << 2 | indexList[i] & 0x03, index);
	fclose(index);
	
	img.release();
	vector<unsigned char>().swap(indexList);
	for (size_t i = 0; i < palList.size(); i++) vector<Color>().swap(palList[i]);
	vector<vector<Color>>().swap(palList);
	return 0;
}
