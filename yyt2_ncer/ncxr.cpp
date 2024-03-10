#include "ncxr.h"

#pragma warning(disable : 4996)

NCLRData::NCLRData()
{
	initial();
}

NCLRData::NCLRData(char *fileName)
{
	read(fileName);
}

NCLRData::NCLRData(unsigned int type, unsigned int pn, unsigned int sn, unsigned int fga, unsigned int fgb)
{
	initial(type, pn, sn, fga, fgb);
}

NCLRData::~NCLRData()
{
	release();
}

void NCLRData::release()
{
	if (data.size() > 0)
	{
		for (unsigned int i = 0; i < data.size(); i++) vector<Color>().swap(data[i]);
		vector<vector<Color>>().swap(data);
	}
	vector<Color>().swap(bgRaw);
}

void NCLRData::initial()
{
	depth = 0;
	sec_num = 0;
	flag_a = 0;
	flag_b = 0;
	bg = { 0, 0, 0, 0 };
	release();
}

void NCLRData::initial(unsigned int type, unsigned int pn, unsigned int sn, unsigned int fga, unsigned int fgb)
{
	depth = type;
	sec_num = sn;
	flag_a = fga;
	flag_b = fgb;
	if (pn == 0) return;
	release();
	switch (depth)
	{
	case 2: vector<vector<Color>>(pn, vector<Color>(4)).swap(data); break;
	case 3: vector<vector<Color>>(pn, vector<Color>(16)).swap(data); break;
	case 4: vector<vector<Color>>(pn, vector<Color>(256)).swap(data); break;
	default:;
	}
}

int NCLRData::write(char *fileName)
{
	unsigned int tpa = 0;
	if (data.size() == 0) return -1;
	FILE *dst = fopen(fileName, "wb");
	if (dst == NULL) return -1;

	vector<unsigned char> raw = PalToByte(data, RGB555, false, true);

	tpa = 0x4E434C52;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x0100FEFF;
	fwrite(&tpa, 4, 1, dst);

	tpa = 0x28 + raw.size();
	if (sec_num == 2) tpa += 0x10 + data.size() * 2;
	fwrite(&tpa, 4, 1, dst);
	tpa = (sec_num << 16) | 0x10;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x504C5454;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x18 + raw.size();
	fwrite(&tpa, 4, 1, dst);
	tpa = (flag_a << 16) | (depth & 0xFFFF);
	fwrite(&tpa, 4, 1, dst);
	fwrite(&flag_b, 4, 1, dst);
	tpa = raw.size();
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x10;
	fwrite(&tpa, 4, 1, dst);
	for (auto i : raw) putc(i, dst);

	if (sec_num == 2)
	{
		tpa = 0x50434D50;
		fwrite(&tpa, 4, 1, dst);
		tpa = 0x10 + data.size() * 2;
		fwrite(&tpa, 4, 1, dst);
		tpa = 0xBEEF0000 | (data.size() & 0xFFFF);
		fwrite(&tpa, 4, 1, dst);
		tpa = 0x08;
		fwrite(&tpa, 4, 1, dst);
		for (unsigned short i = 0; i < data.size(); i++) fwrite(&i, 2, 1, dst);
	}
	fclose(dst);
	return 0;
}

int NCLRData::read(char *fileName)
{
	initial();
	unsigned int tpa = 0, tpb = 0, cn = 0, pn = 0;
	FILE *src = fopen(fileName, "rb");
	if (src == NULL) return -1;
	fseek(src, 0, 2);
	unsigned int fsize = ftell(src);
	rewind(src);
	if (fsize < 0x28)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x4E434C52)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	if (fsize < tpa)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	sec_num = (tpa >> 16) & 0xFFFF;
	if (sec_num != 1 && sec_num != 2)
	{
		fclose(src);
		return -1;
	}

	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	depth = tpa & 0xFFFF;
	flag_a = (tpa >> 16) & 0xFFFF;
	fread(&flag_b, 4, 1, src);
	fread(&tpa, 4, 1, src);
	fread(&tpb, 4, 1, src);
	if (tpa < 2)
	{
		fclose(src);
		return -1;
	}

	switch (depth)
	{
	case 3: cn = 16; break;
	case 4: cn = 256; break;
	default: fclose(src); return -1;
	}
	pn = tpa / (cn * 2);
	if ((pn > 0 && tpa % cn != 0) || (cn == 16 && pn > 16) || (cn == 256 && pn > 1))
	{
		fclose(src);
		return -1;
	}
	if (pn == 0) pn = 1;

	vector<unsigned char> raw;
	while (tpa--) raw.push_back(getc(src));

	vector<unsigned short> id;
	if (sec_num == 2)
	{
		fread(&tpa, 4, 1, src);
		fread(&tpa, 4, 1, src);
		fread(&tpa, 4, 1, src);
		fread(&tpb, 4, 1, src);
		tpa = tpa & 0xFFFF;
		if (pn != tpa)
		{
			fclose(src);
			return -1;
		}
		vector<unsigned short>(pn).swap(id);
		for (unsigned int i = 0; i < pn; i++) fread(&id[i], 2, 1, src);
		tpb = 0;
		for (auto i : id) if (i > tpb) tpb = i;
		if ((cn == 256 && tpb >= 1) || (cn == 16 && tpb >= 16))
		{
			fclose(src);
			return -1;
		}
	}
	else for (unsigned short i = 0; i < pn; i++) id.push_back(i);

	vector<vector<Color>> col = ByteToPal(raw, RGB555, cn, pn, false, true);
	vector<vector<Color>>(pn, vector<Color>(cn)).swap(data);
	for (unsigned int i = 0; i < id.size(); i++) for (unsigned int j = 0; j < col[i].size(); j++) data[id[i]][j] = col[i][j];

	for (unsigned int i = 0; i < col.size(); i++) vector<Color>().swap(col[i]);
	vector<vector<Color>>().swap(col);
	vector<unsigned char>().swap(raw);
	return 0;
}

Color NCLRData::get(unsigned int pi, unsigned int ci) const
{
	if (pi >= data.size() || ci >= data[0].size()) return { 0,0,0,0 };
	return data[pi][ci];
}

void NCLRData::set(Color col, unsigned int pi, unsigned int ci)
{
	if (pi >= data.size() || ci >= data[0].size()) return;
	data[pi][ci] = col;
}

int NCLRData::depthConv(unsigned int d)
{
	if (d == depth) return 0;
	if (data.size() == 0) return -1;
	vector<vector<Color>> rec;
	if (d == 3)
	{
		if (data[0].size() < 0x100) return -1;
		vector<vector<Color>>(0x10, vector<Color>(0x10, { 0, 0, 0, 0 })).swap(rec);
		for (unsigned int i = 0; i < 0x10; i++) for (unsigned int j = 0; j < 0x10; j++) rec[i][j] = data[0][i * 0x10 + j];
	}
	else if (d == 4)
	{
		if (data.size() < 0x10) return -1;
		vector<vector<Color>>(1, vector<Color>(0x100, { 0, 0, 0, 0 })).swap(rec);
		for (unsigned int i = 0; i < 0x10; i++) for (unsigned int j = 0; j < data[i].size(); j++) rec[0][i * 0x10 + j] = data[i][j];
	}
	else return -1;
	depth = d;
	rec.swap(data);
	for (unsigned int i = 0; i < rec.size(); i++) vector<Color>().swap(rec[i]);
	vector<vector<Color>>().swap(rec);
	return 0;
}

Color NCLRData::setAlpha()
{
	vector<Color> colRaw;
	for (auto term : data) colRaw.push_back(term[0]);
	colRaw.push_back({ 0, 0, 0, 0xFF });
	colRaw.push_back({ 0, 0, 0xFF, 0xFF });
	colRaw.push_back({ 0, 0xFF, 0, 0xFF });
	colRaw.push_back({ 0xFF, 0, 0, 0xFF });
	colRaw.push_back({ 0xFF, 0xFF, 0, 0xFF });
	colRaw.push_back({ 0, 0xFF, 0xFF, 0xFF });
	colRaw.push_back({ 0xFF, 0, 0xFF, 0xFF });
	colRaw.push_back({ 0xFF, 0xFF, 0xFF, 0xFF });
	colRaw.push_back({ 0x80, 0x80, 0x80, 0xFF });
	map<unsigned int, Color> bgcs;
	for (auto term : colRaw) bgcs[getKey(term)] = term;
	map<unsigned int, Color> cols;
	for (size_t i = 0; i < data.size(); i++) for (size_t j = 1; j < data[i].size(); j++)
	{
		unsigned int ck = getKey(data[i][j]);
		if (cols.find(ck) == cols.end()) cols[ck] = data[i][j];
		if (bgcs.find(ck) != bgcs.end()) bgcs.erase(ck);
	}
	if (bgcs.size() > 0)
	{
		for (auto term : colRaw) if (bgcs.find(getKey(term)) != bgcs.end())
		{
			bg = term;
			break;
		}
	}
	else
	{
		unsigned int r = 0, g = 0, b = 0;
		for (map<unsigned int, Color>::iterator iter = cols.begin(); iter != cols.end(); iter++)
		{
			r += iter->second.r;
			g += iter->second.g;
			b += iter->second.b;
		}
		bg = getInvc({ (r / cols.size()) & 0xFF, (g / cols.size()) & 0xFF, (b / cols.size()) & 0xFF, 0xFF });
	}
	vector<Color>(data.size()).swap(bgRaw);
	for (size_t i = 0; i < data.size(); i++)
	{
		bgRaw[i] = data[i][0];
		data[i][0] = bg;
	}
	vector<Color>().swap(colRaw);
	return bg;
}

unsigned int NCLRData::getFlagA()
{
	return flag_a;
}

unsigned int NCLRData::getFlagB()
{
	return flag_b;
}

unsigned int NCLRData::getDepth()
{
	return depth;
}

unsigned int NCLRData::getPalNum()
{
	return data.size();
}

unsigned int NCLRData::getColNum()
{
	if (data.size() == 0) return 0;
	return data[0].size();
}

unsigned int NCLRData::getSecNum()
{
	return sec_num;
}

unsigned int NCLRData::getKey(Color c)
{
	return (c.a << 24) + (c.r << 16) + (c.g << 8) + c.b;
}

Color NCLRData::getInvc(Color c)
{
	return { (unsigned char)(0xFF - c.r), (unsigned char)(0xFF - c.g), (unsigned char)(0xFF - c.b), c.a };
}

NCGRData::NCGRData()
{
	initial();
}

NCGRData::NCGRData(char *fileName)
{
	read(fileName);
}

NCGRData::NCGRData(unsigned int type, unsigned int w, unsigned int h, unsigned int wb, unsigned int hb, unsigned int fg, unsigned int sn, unsigned int cs, unsigned int cc)
{
	initial(type, w, h, wb, hb, fg, sn, cs, cc);
}

NCGRData::~NCGRData()
{
	release();
}

void NCGRData::release()
{
	if (data.size() > 0)
	{
		for (unsigned int i = 0; i < data.size(); i++) vector<unsigned char>().swap(data[i]);
		vector<vector<unsigned char>>().swap(data);
	}
}

void NCGRData::initial()
{
	full = 0;
	depth = 0;
	width = 0;
	height = 0;
	width_b = 0;
	height_b = 0;
	flag = 0;
	sec_num = 0;
	char_size = 0;
	char_count = 0;
	release();
}

void NCGRData::initial(unsigned int type, unsigned int w, unsigned int h, unsigned int wb, unsigned int hb, unsigned int fg, unsigned int sn, unsigned int cs, unsigned int cc)
{
	depth = type;
	width = w;
	height = h;
	width_b = wb;
	height_b = hb;
	flag = fg;
	sec_num = sn;
	char_size = cs;
	char_count = cc;
	release();
}

int NCGRData::read(char *fileName)
{
	initial();
	unsigned int tpa = 0, tpb = 0;
	FILE *src = fopen(fileName, "rb");
	if (src == NULL) return -1;
	fseek(src, 0, 2);
	unsigned int fsize = ftell(src);
	rewind(src);
	if (fsize < 0x30)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x4E434752)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	if (fsize < tpa)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	sec_num = (tpa >> 16) & 0xFFFF;
	if (sec_num != 1 && sec_num != 2)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x43484152)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	height = tpa & 0xFFFF;
	width = (tpa >> 16) & 0xFFFF;
	fread(&depth, 4, 1, src);
	fread(&tpa, 4, 1, src);
	height_b = tpa & 0xFFFF;
	width_b = (tpa >> 16) & 0xFFFF;
	fread(&flag, 4, 1, src);
	if (flag > 1)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpb, 4, 1, src);
	tpb = depth == 3 ? 32 : 64;
	if (tpa % tpb != 0)
	{
		fclose(src);
		return -1;
	}
	tpa /= tpb;
	if (flag == 1)
	{
		if (getWidth() == 0)
		{
			fclose(src);
			return -1;
		}
		unsigned char srcMode = depth == 3 ? 5 : 6;
		unsigned int srcTileCol = depth == 3 ? getWidth() * 4 : getWidth() * 8;
		vector<vector<unsigned char>>(tpa*tpb, vector<unsigned char>(1)).swap(data);
		for (size_t i = 0; i < data.size(); i++) data[i][0] = getc(src);
		vector<vector<unsigned char>> tmp = TileConv(data, srcTileCol, srcMode, 0, depth - 3);
		tmp.swap(data);
		for (unsigned int i = 0; i < tmp.size(); i++) vector<unsigned char>().swap(tmp[i]);
		vector<vector<unsigned char>>().swap(tmp);
	}
	else
	{
		vector<vector<unsigned char>>(tpa, vector<unsigned char>(tpb)).swap(data);
		for (unsigned int i = 0; i < tpa; i++) for (unsigned int j = 0; j < tpb; j++) data[i][j] = getc(src);
	}
	if (data.size() == 0)
	{
		fclose(src);
		return -1;
	}
	if (sec_num == 2)
	{
		fread(&tpa, 4, 1, src);
		fread(&tpa, 4, 1, src);
		fread(&tpa, 4, 1, src);
		fread(&tpa, 4, 1, src);
		char_size = tpa & 0xFFFF;
		char_count = (tpa >> 16) & 0xFFFF;
	}
	return 0;
}

int NCGRData::write(char *fileName)
{
	unsigned int tpa = 0;
	if (data.size() == 0) return -1;
	FILE *dst = fopen(fileName, "wb");
	if (dst == NULL) return -1;

	tpa = 0x4E434752;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x0101FEFF;
	fwrite(&tpa, 4, 1, dst);

	tpa = 0x30 + data.size()*data[0].size();
	if (sec_num == 2) tpa += 0x10;
	fwrite(&tpa, 4, 1, dst);
	tpa = (sec_num << 16) | 0x10;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x43484152;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x20 + data.size()*data[0].size();
	fwrite(&tpa, 4, 1, dst);
	tpa = (width << 16) | (height & 0xFFFF);
	fwrite(&tpa, 4, 1, dst);
	fwrite(&depth, 4, 1, dst);
	tpa = (width_b << 16) | (height_b & 0xFFFF);
	fwrite(&tpa, 4, 1, dst);
	fwrite(&flag, 4, 1, dst);
	tpa = data.size()*data[0].size();
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x18;
	fwrite(&tpa, 4, 1, dst);
	if (flag == 1)
	{
		unsigned char dstMode = depth == 3 ? 5 : 6;
		vector<vector<unsigned char>> tmp = TileConv(data, getWidth(), 0, dstMode, depth - 3);
		for (unsigned int i = 0; i < tmp.size(); i++) for (unsigned int j = 0; j < tmp[i].size(); j++) putc(tmp[i][j], dst);
	}
	else for (unsigned int i = 0; i < data.size(); i++) for (unsigned int j = 0; j < data[i].size(); j++) putc(data[i][j], dst);
	if (sec_num == 2)
	{
		tpa = 0x43504F53;
		fwrite(&tpa, 4, 1, dst);
		tpa = 0x10;
		fwrite(&tpa, 4, 1, dst);
		tpa = 0x00;
		fwrite(&tpa, 4, 1, dst);
		tpa = (char_count << 16) | (char_size & 0xFFFF);
		fwrite(&tpa, 4, 1, dst);
	}
	fclose(dst);
	return 0;
}

void NCGRData::add(vector<unsigned char> tile)
{
	data.push_back(vector<unsigned char>(tile.size()));
	for (size_t i = 0; i < tile.size(); i++) data[data.size() - 1][i] = tile[i];
}

vector<unsigned char> NCGRData::get(unsigned int id) const
{
	if (id >= data.size()) return vector<unsigned char>();
	return data[id];
}

void NCGRData::set(vector<unsigned char> tile, unsigned int id)
{
	if (id < data.size())
	{
		vector<unsigned char>().swap(data[id]);
		data[id] = tile;
	}
}

void NCGRData::clone(NCGRData &dst) const
{
	dst.initial(depth, width, height, width_b, height_b, flag, sec_num, char_size, char_count);
	dst.full = full;
	vector<vector<unsigned char>>(data.size()).swap(dst.data);
	for (size_t i = 0; i < data.size(); i++) for (size_t j = 0; j < data[i].size(); j++) dst.data[i].push_back(data[i][j]);
}

unsigned int NCGRData::getWidth()
{
	return width == 0xFFFF ? width_b : width;
}

unsigned int NCGRData::getHeight()
{
	return height == 0xFFFF ? height_b : height;
}

unsigned int NCGRData::getDepth()
{
	return depth;
}

unsigned int NCGRData::getFlag()
{
	return flag;
}

unsigned int NCGRData::getSecNum()
{
	return sec_num;
}

unsigned int NCGRData::getTileNum()
{
	return data.size();
}

unsigned int NCGRData::getCS()
{
	return char_size;
}

unsigned int NCGRData::getCC()
{
	return char_count;
}

NSCRData::NSCRData()
{
	initial();
}

NSCRData::NSCRData(char *fileName)
{
	read(fileName);
}

NSCRData::NSCRData(unsigned int w, unsigned int h, unsigned int is, unsigned int bt)
{
	initial(w, h, is, bt);
}

NSCRData::~NSCRData()
{
	release();
}

void NSCRData::release()
{
	if (data.size() > 0) vector<unsigned short>().swap(data);
}

void NSCRData::initial()
{
	width = 0;
	height = 0;
	is_size = 0;
	bg_type = 0;
	release();
}

void NSCRData::initial(unsigned int w, unsigned int h, unsigned int is, unsigned int bt)
{
	width = w;
	height = h;
	is_size = is;
	bg_type = bt;
	release();
	vector<unsigned short>(w * h / 64).swap(data);
}

int NSCRData::write(char *fileName)
{
	unsigned int tpa = 0;
	if (data.size() == 0) return -1;
	FILE *dst = fopen(fileName, "wb");
	if (dst == NULL) return -1;
	tpa = 0x4E534352;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x0101FEFF;
	fwrite(&tpa, 4, 1, dst);

	tpa = bg_type > 0 ? 0x24 + data.size() : 0x24 + data.size() * 2;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x00010010;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x5343524E;
	fwrite(&tpa, 4, 1, dst);
	tpa = bg_type > 0 ? 0x14 + data.size() : 0x14 + data.size() * 2;
	fwrite(&tpa, 4, 1, dst);
	tpa = (height_raw << 16) | (width_raw & 0xFFFF);
	fwrite(&tpa, 4, 1, dst);
	tpa = (bg_type << 16) | (is_size & 0xFFFF);
	fwrite(&tpa, 4, 1, dst);
	tpa = bg_type > 0 ? data.size() : data.size() * 2;
	fwrite(&tpa, 4, 1, dst);
	if (bg_type == 0) for (unsigned int i = 0; i < data.size(); i++) fwrite(&data[i], 2, 1, dst);
	else for (unsigned int i = 0; i < data.size(); i++) putc(data[i] & 0xFF, dst);
	fclose(dst);
	return 0;
}

int NSCRData::read(char *fileName)
{
	initial();
	unsigned int tpa = 0, tpb = 0;
	FILE *src = fopen(fileName, "rb");
	if (src == NULL) return -1;
	fseek(src, 0, 2);
	unsigned int fsize = ftell(src);
	rewind(src);
	if (fsize < 0x24)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x4E534352)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	if (fsize < tpa)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x00010010)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	width_raw = tpa & 0xFFFF;
	width = width_raw;
	height_raw = (tpa >> 16) & 0xFFFF;
	height = height_raw;
	if (width == 512)
	{
		width /= 2;
		height *= 2;
	}
	fread(&tpa, 4, 1, src);
	is_size = tpa & 0xFFFF;
	bg_type = (tpa >> 16) & 0xFFFF;
	fread(&tpa, 4, 1, src);
	if ((bg_type > 0 && tpa != width * height / 64) || (bg_type == 0 && tpa != width * height / 32) || width % 8 != 0 || height % 8 != 0)
	{
		fclose(src);
		return -1;
	}
	vector<unsigned short>(width * height / 64).swap(data);
	if (bg_type == 0) for (unsigned int i = 0; i < data.size(); i++) fread(&data[i], 2, 1, src);
	else for (unsigned int i = 0; i < data.size(); i++) data[i] = getc(src);
	return 0;
}

int NSCRData::modify(char *fileName, NCGRData &ncgr, NCLRData &nclr, bool keepTile)
{
	ImageData imgMod(fileName);
	if (imgMod.width != width || imgMod.height != height) return -1;
	ImageData imgOri;
	if (TMPToImage(imgOri, ncgr.data, data, nclr.data, width, height, 0, ncgr.getDepth() - 3) == -1)
	{
		imgMod.release();
		return -1;
	}
	bool test = true;
	for (size_t i = 0; i < imgMod.size; i++) if (imgMod.get(i) != imgOri.get(i))
	{
		test = false;
		break;
	}
	if (test)
	{
		imgMod.release();
		return -1;
	}
	if (!keepTile)
	{
		for (size_t i = 0; i < ncgr.data.size(); i++) vector<unsigned char>().swap(ncgr.data[i]);
		vector<vector<unsigned char>>().swap(ncgr.data);
	}
	for (size_t i = 0; i < data.size(); i++)
	{
		unsigned int x = i % (imgMod.width / 8) * 8, y = i / (imgMod.width / 8) * 8;
		if (keepTile && imgMod.clip(x, y, 8, 8) == imgOri.clip(x, y, 8, 8)) continue;
		test = true;
		int palId[4];
		vector<vector<unsigned char>> tileData(4);
		for (int j = 0; j < 4; j++)
		{
			ImageData tile = imgMod.clip(x, y, 8, 8).flip(j);
			palId[j] = PalMatch(tile, nclr.data, tileData[j], ncgr.getDepth() - 3, getPalId(i));
			int res = TileMatch(vector<vector<unsigned char>>(1, tileData[j]), ncgr.data);
			if (res >= 0)
			{
				data[i] = (palId[j] & 0xF) << 12 | (j & 3) << 10 | (res & 0x3FF);
				test = false;
				tile.release();
				break;
			}
			tile.release();
		}
		if (test)
		{
			if (ncgr.getTileNum() > 0x3FF)
			{
				ncgr.full = 1;
				for (size_t j = 0; j < 4; j++) vector<unsigned char>().swap(tileData[j]);
				vector<vector<unsigned char>>().swap(tileData);
				imgOri.release();
				imgMod.release();
				return -1;
			}
			data[i] = (palId[0] & 0xF) << 12 | (ncgr.getTileNum() & 0x3FF);
			ncgr.add(tileData[0]);
		}
		for (size_t j = 0; j < tileData.size(); j++) vector<unsigned char>().swap(tileData[j]);
		vector<vector<unsigned char>>().swap(tileData);
	}
	imgOri.release();
	imgMod.release();
	return 0;
}

unsigned short NSCRData::get(unsigned int id) const
{
	if (id >= data.size()) return 0;
	return data[id];
}

unsigned int NSCRData::getFlip(unsigned int id) const
{
	if (id >= data.size()) return 0;
	return (data[id] >> 10) & 3;
}

unsigned int NSCRData::getTileId(unsigned int id) const
{
	if (id >= data.size()) return 0;
	return data[id] & 0x3FF;
}

unsigned int NSCRData::getPalId(unsigned int id) const
{
	if (id >= data.size()) return 0;
	return  (data[id] >> 12) & 0xF;
}

void NSCRData::set(unsigned short info, unsigned int id)
{
	if (id < data.size()) data[id] = info;
}

unsigned int NSCRData::getWidth()
{
	return width;
}

unsigned int NSCRData::getHeight()
{
	return height;
}

unsigned int NSCRData::getSize()
{
	return width * height;
}

unsigned int NSCRData::getISSize()
{
	return is_size;
}

unsigned int NSCRData::getBGType()
{
	return bg_type;
}

unsigned int NSCRData::getInfoNum()
{
	return data.size();
}

OAMData::OAMData()
{
	initial();
}

OAMData::OAMData(const vector<unsigned short> &attrs, unsigned int flags, int lay)
{
	set(attrs, flags, lay);
}

void OAMData::initial()
{
	xpos = 0;
	ypos = 0;
	width = 0;
	height = 0;
	rs_flag = 0;
	aa_flag = 0;
	ab_flag = 0;
	ac_flag = 0;
	ad_flag = 0;
	ds_flag = 0;
	depth = 0;
	shape = 0;
	size = 0;
	hv_mode = 0;
	tile_index = 0;
	pal_index = 0;
	shift = 0;
	layer = -1;
}

void OAMData::set(const vector<unsigned short> &attrs, unsigned int flags, int lay)
{
	layer = lay;
	ypos = attrs[0] & 0xFF;
	if (ypos >= 0x80) ypos -= 0x100;
	xpos = attrs[1] & 0x1FF;
	if (xpos >= 0x100) xpos -= 0x200;
	shape = (attrs[0] >> 14) & 3;
	size = (attrs[1] >> 14) & 3;
	rs_flag = (attrs[0] >> 8) & 1;
	hv_mode = (attrs[1] >> 12) & 3;
	depth = (attrs[0] >> 13) & 1;
	aa_flag = (attrs[0] >> 10) & 3;
	ab_flag = (attrs[0] >> 12) & 1;
	ac_flag = (attrs[1] >> 9) & 7;
	ad_flag = (attrs[2] >> 10) & 3;
	ds_flag = (attrs[0] >> 9) & 1;
	tile_index = attrs[2] & 0x3FF;
	pal_index = (attrs[2] >> 12) & 0x0F;
	width = calcWidth();
	height = calcHeight();
	shift = (flags & 3) > 0 ? 1 : 0;
	for (unsigned int i = 0; i < shift; i++) tile_index *= 2;
}

vector<unsigned short>  OAMData::get() const
{
	int temp = 0;
	vector<unsigned short> attrs(3, 0);
	temp = ypos < 0 ? ypos + 0x100 : ypos;
	attrs[0] = (shape & 3) << 14 | (depth & 1) << 13 | (ab_flag & 1) << 12 | (aa_flag & 3) << 10 | (ds_flag & 1) << 9 | (rs_flag & 1) << 8 | (temp & 0xFF);
	temp = xpos < 0 ? xpos + 0x200 : xpos;
	attrs[1] = (size & 3) << 14 | (hv_mode & 3) << 12 | (ac_flag & 7) << 9 | (temp & 0x1FF);
	temp = tile_index;
	for (unsigned int i = 0; i < shift; i++) temp /= 2;
	attrs[2] = (pal_index & 0x0F) << 12 | (ad_flag & 3) << 10 | (temp & 0x3FF);
	return attrs;
}

vector<unsigned short> OAMData::toMap(unsigned int tileCol)
{
	vector<unsigned short> rec;
	if (tileCol == 0 || tileCol == 0xFFFF) tileCol = 1;
	unsigned int tc = width / 8;
	unsigned int rc = tileCol >= tc ? tc : tileCol;
	unsigned int pc = tileCol - rc;
	for (unsigned int i = 0; i < width * height / (unsigned int)0x40; i++) rec.push_back(((pal_index & 0xF) << 12) + (tile_index + i + (i / rc) * pc & 0x3FF));
	return rec;
}

int OAMData::calcWidth()
{
	int width = 0;
	switch (shape)
	{
	case 0x00:
		switch (size)
		{
		case 0x00:
			width = 8;
			break;
		case 0x01:
			width = 16;
			break;
		case 0x02:
			width = 32;
			break;
		case 0x03:
			width = 64;
			break;
		}
		break;
	case 0x01:
		switch (size)
		{
		case 0x00:
			width = 16;
			break;
		case 0x01:
			width = 32;
			break;
		case 0x02:
			width = 32;
			break;
		case 0x03:
			width = 64;
			break;
		}
		break;
	case 0x02:
		switch (size)
		{
		case 0x00:
			width = 8;
			break;
		case 0x01:
			width = 8;
			break;
		case 0x02:
			width = 16;
			break;
		case 0x03:
			width = 32;
			break;
		}
		break;
	}
	return width;
}

int OAMData::calcHeight()
{
	int height = 0;
	switch (shape)
	{
	case 0x00:
		switch (size)
		{
		case 0x00:
			height = 8;
			break;
		case 0x01:
			height = 16;
			break;
		case 0x02:
			height = 32;
			break;
		case 0x03:
			height = 64;
			break;
		}
		break;
	case 0x01:
		switch (size)
		{
		case 0x00:
			height = 8;
			break;
		case 0x01:
			height = 8;
			break;
		case 0x02:
			height = 16;
			break;
		case 0x03:
			height = 32;
			break;
		}
		break;
	case 0x02:
		switch (size)
		{
		case 0x00:
			height = 16;
			break;
		case 0x01:
			height = 32;
			break;
		case 0x02:
			height = 32;
			break;
		case 0x03:
			height = 64;
			break;
		}
		break;
	}
	return height;
}

OAMData OAMData::operator = (const OAMData &src)
{
	layer = src.layer;
	ypos = src.ypos;
	xpos = src.xpos;
	shape = src.shape;
	size = src.size;
	rs_flag = src.rs_flag;
	hv_mode = src.hv_mode;
	depth = src.depth;
	aa_flag = src.aa_flag;
	ab_flag = src.ab_flag;
	ac_flag = src.ac_flag;
	ad_flag = src.ad_flag;
	ds_flag = src.ds_flag;
	tile_index = src.tile_index;
	pal_index = src.pal_index;
	width = src.width;
	height = src.height;
	shift = src.shift;
}

OAMData OAMData::clone() const
{
	OAMData res;
	res.layer = layer;
	res.ypos = ypos;
	res.xpos = xpos;
	res.shape = shape;
	res.size = size;
	res.rs_flag = rs_flag;
	res.hv_mode = hv_mode;
	res.depth = depth;
	res.aa_flag = aa_flag;
	res.ab_flag = ab_flag;
	res.ac_flag = ac_flag;
	res.ad_flag = ad_flag;
	res.ds_flag = ds_flag;
	res.tile_index = tile_index;
	res.pal_index = pal_index;
	res.width = width;
	res.height = height;
	res.shift = shift;
	return res;
}

OAMBank::OAMBank()
{
	initial();
}

OAMBank::OAMBank(unsigned int num, unsigned int exf, unsigned int ukf, short xs, short xl, short ys, short yl)
{
	initial(num, exf, ukf, xs, xl, ys, yl);
}

OAMBank::~OAMBank()
{
	release();
}

void OAMBank::release()
{
	img.release();
	if (data.size() > 0) vector<OAMData>().swap(data);
}

void OAMBank::initial()
{
	ext_flag = 0;
	ext_data[0] = 0;
	ext_data[1] = 0;
	uk_flag = 0;
	x_min = 0;
	x_max = 0;
	y_min = 0;
	y_max = 0;
	width = 0;
	height = 0;
	x_pos = 0;
	y_pos = 0;
	bg = { 0, 0, 0, 0 };
	release();
}

void OAMBank::initial(unsigned int num, unsigned int exf, unsigned int ukf, short xs, short xl, short ys, short yl)
{
	ext_flag = exf;
	ext_data[0] = 0;
	ext_data[1] = 0;
	uk_flag = ukf;
	x_min = xs;
	x_max = xl;
	y_min = ys;
	y_max = yl;
	width = 0;
	height = 0;
	x_pos = 0;
	y_pos = 0;
	release();
	vector<OAMData>(num).swap(data);
}

void OAMBank::calcSize()
{
	x_pos = 0;
	y_pos = 0;
	width = 0;
	height = 0;
	if (data.size() == 0) return;
	for (auto term : data)
	{
		if (x_pos > term.xpos) x_pos = term.xpos;
		if (width < term.xpos + term.width) width = term.xpos + term.width;
		if (y_pos > term.ypos) y_pos = term.ypos;
		if (height < term.ypos + term.height) height = term.ypos + term.height;
	}
	width = width - x_pos;
	height = height - y_pos;
	while (width % 4 != 0) width += 1;
	while (height % 4 != 0) height += 1;
}

void OAMBank::resetImg()
{
	if (width == 0 || height == 0) return;
	img.initial(width, height, { 0, 0, 0, 0 }, 1);
}

void OAMBank::add(const vector<unsigned short> &attrs, unsigned int flags, unsigned int lay)
{
	data.push_back(OAMData(attrs, flags, lay));
}

void OAMBank::add(const OAMData &term)
{
	data.push_back(term.clone());
}

void OAMBank::set(unsigned int id, const vector<unsigned short> &attrs, unsigned int flags, unsigned int lay)
{
	if (id < data.size()) data[id].set(attrs, flags, lay);
}

void OAMBank::set(unsigned int id, const OAMData &term)
{
	if (id < data.size()) data[id] = term.clone();
}

OAMData OAMBank::get(unsigned int id)
{
	OAMData rec;
	if (id < data.size()) rec = data[id];
	return rec;
}

void OAMBank::clone(OAMBank &dst) const
{
	dst.initial(data.size(), ext_flag, uk_flag, x_min, x_max, y_min, y_max);
	for (unsigned int i = 0; i < data.size(); i++) dst.set(i, data[i]);
	dst.width = width;
	dst.height = height;
	dst.x_pos = x_pos;
	dst.y_pos = y_pos;
}

int OAMBank::draw(unsigned int id, NCGRData &ncgr, NCLRData &nclr, bool cover)
{
	if (data[id].pal_index > nclr.data.size()) data[id].pal_index = 0;
	unsigned int tileCol = ncgr.getFlag() != 0 ? ncgr.getWidth() : 0;
	ImageData ta;
	TMPToImage(ta, ncgr.data, data[id].toMap(tileCol), nclr.data, data[id].width, data[id].height, 0, ncgr.getDepth() - 3);
	ImageData tb = ta.flip(data[id].hv_mode);
	int rec = img.paste(tb, data[id].xpos - x_pos, data[id].ypos - y_pos, cover);
	ta.release();
	tb.release();
	return rec;
}

int OAMBank::modify(int layer, vector<OAMData> &oamList, char *fileName, NCGRData &ncgr, NCLRData &nclr, bool keepTile)
{
	for (size_t i = 0; i < img.size; i++) if (img.get(i).a == 0) img.set(bg, i);
	ImageData imgMod(fileName);
	if (imgMod.width != width || imgMod.height != height) img.clone(imgMod);
	if (keepTile)
	{
		bool test = true;
		for (size_t i = 0; i < imgMod.size; i++) if (imgMod.get(i) != img.get(i))
		{
			test = false;
			break;
		}
		if (test)
		{
			for (size_t i = 0; i < data.size(); i++) if (data[i].layer == layer) oamList.push_back(data[i].clone());
			imgMod.release();
			return -1;
		}
	}
	for (size_t i = 0; i < data.size(); i++)
	{
		if (data[i].layer != layer) continue;
		int x = data[i].xpos - x_pos, y = data[i].ypos - y_pos;
		if (keepTile && imgMod.clip(x, y, data[i].width, data[i].height) == img.clip(x, y, data[i].width, data[i].height))
		{
			oamList.push_back(data[i].clone());
			continue;
		}
		bool test = true;
		int palId[4];
		vector<vector<vector<unsigned char>>> tileData(4);
		for (int j = 0; j < 4; j++)
		{
			ImageData tile;
			imgMod.clip(x, y, data[i].width, data[i].height).flip(j).clone(tile);
			if (j == -1)
			{
				char tt[260];
				sprintf(tt, "test/%d_%d_%d_%d_%d.bmp", i, x, y, data[i].width, data[i].height);
				tile.write(tt);
			}
			vector<unsigned char> ta;
			palId[j] = PalMatch(tile, nclr.data, ta, ncgr.getDepth() - 3, data[i].pal_index);
			tile.release();
			vector<vector<unsigned char>> tb(ta.size(), vector<unsigned char>(1, 0));
			for (size_t t = 0; t < ta.size(); t++) tb[t][0] = ta[t];
			unsigned char srcMode = ncgr.getDepth() == 3 ? 5 : 6;
			unsigned int srcTileCol = ncgr.getDepth() == 3 ? data[i].width / 2 : data[i].width;
			TileConv(tb, srcTileCol, srcMode, 0, ncgr.getDepth() - 3).swap(tileData[j]);
			int res = TileMatch(tileData[j], ncgr.data, data[i].shift * 2);
			if (res >= 0)
			{
				data[i].rs_flag = 1;
				data[i].tile_index = res;
				data[i].pal_index = palId[j];
				data[i].hv_mode = j;
				oamList.push_back(data[i].clone());
				test = false;
				break;
			}
			vector<unsigned char>().swap(ta);
			for (size_t t = 0; t < tb.size(); t++) vector<unsigned char>().swap(tb[i]);
			vector<vector<unsigned char>>().swap(tb);
		}
		if (test)
		{
			test = false;
			int tileWidth = 8, tileHeight = 8, shape = 0, size = 0;
			if (keepTile)
			{
				if (data[i].shift > 0)
				{
					if (data[i].width % 16 == 0)
					{
						tileWidth = 16;
						shape = 1;
						size = 0;
						test = true;
					}
					else if (data[i].height % 16 == 0)
					{
						tileHeight = 16;
						shape = 2;
						size = 0;
						test = true;
					}
				}
			}
			if (test)
			{
				for (int tid = 0; tid < (data[i].width / tileWidth) * (data[i].height / tileHeight); tid++)
				{
					OAMData oamTile = data[i].clone();
					int tileX = x + tid % (data[i].width / tileWidth) * tileWidth, tileY = y + tid / (data[i].width / tileWidth) * tileHeight;
					oamTile.xpos = tileX + x_pos;
					oamTile.ypos = tileY + y_pos;
					oamTile.shape = shape;
					oamTile.size = size;
					test = true;
					for (size_t j = 0; j < tileData.size(); j++)
					{
						for (size_t t = 0; t < tileData[j].size(); t++) vector<unsigned char>().swap(tileData[j][t]);
						vector<vector<unsigned char>>().swap(tileData[j]);
					}
					for (int j = 0; j < 4; j++)
					{
						ImageData tile;
						imgMod.clip(tileX, tileY, tileWidth, tileHeight).flip(j).clone(tile);
						vector<unsigned char> ta;
						palId[j] = PalMatch(tile, nclr.data, ta, ncgr.getDepth() - 3, data[i].pal_index);
						tile.release();
						vector<vector<unsigned char>> tb(ta.size(), vector<unsigned char>(1, 0));
						for (size_t t = 0; t < ta.size(); t++) tb[t][0] = ta[t];
						unsigned char srcMode = ncgr.getDepth() == 3 ? 5 : 6;
						unsigned int srcTileCol = ncgr.getDepth() == 3 ? tileWidth / 2 : tileWidth;
						TileConv(tb, srcTileCol, srcMode, 0, ncgr.getDepth() - 3).swap(tileData[j]);
						int res = TileMatch(tileData[j], ncgr.data, data[i].shift * 2);
						if (res >= 0)
						{
							oamTile.rs_flag = 1;
							oamTile.tile_index = res;
							oamTile.pal_index = palId[j];
							oamTile.hv_mode = j;
							oamList.push_back(oamTile.clone());
							test = false;
							break;
						}
						vector<unsigned char>().swap(ta);
						for (size_t t = 0; t < tb.size(); t++) vector<unsigned char>().swap(tb[i]);
						vector<vector<unsigned char>>().swap(tb);
					}
					if (test)
					{
						if (data[i].shift > 0 && tileData[0].size() % 2 != 0) tileData[0].push_back(vector<unsigned char>(tileData[0][0].size(), 0));
						if (ncgr.getTileNum() + tileData[0].size() <= 0x400)
						{
							oamTile.tile_index = ncgr.getTileNum();
							for (auto term : tileData[0]) ncgr.add(term);
						}
						else
						{
							ncgr.full = 1;
							oamTile.tile_index = 0;
							oamTile.shape = 0;
							oamTile.size = 0;
						}
						oamTile.pal_index = palId[0];
						oamTile.hv_mode = 0;
						oamList.push_back(oamTile.clone());
					}
				}
			}
			else
			{
				if (data[i].shift > 0 && tileData[0].size() % 2 != 0) tileData[0].push_back(vector<unsigned char>(tileData[0][0].size(), 0));
				if (ncgr.getTileNum() + tileData[0].size() <= 0x400)
				{
					data[i].tile_index = ncgr.getTileNum();
					for (auto term : tileData[0]) ncgr.add(term);
				}
				else
				{
					ncgr.full = 1;
					data[i].tile_index = 0;
					data[i].shape = 0;
					data[i].size = 0;
				}
				data[i].pal_index = palId[0];
				data[i].hv_mode = 0;
				oamList.push_back(data[i].clone());
			}
		}
		for (size_t j = 0; j < tileData.size(); j++)
		{
			for (size_t t = 0; t < tileData[j].size(); t++) vector<unsigned char>().swap(tileData[j][t]);
			vector<vector<unsigned char>>().swap(tileData[j]);
		}
		vector<vector<vector<unsigned char>>>().swap(tileData);
	}
	imgMod.release();
	return 0;
}

void OAMBank::write(char *fileName)
{
	for (size_t i = 0; i < img.size; i++) if (img.get(i).a == 0) img.set(bg, i);
	img.write(fileName);
}

NCERData::NCERData()
{
	initial();
}

NCERData::NCERData(char *fileName)
{
	read(fileName);
}

NCERData::NCERData(unsigned int num, unsigned int flg, unsigned int exf)
{
	initial(num, flg, exf);
}

NCERData::~NCERData()
{
	release();
}

void NCERData::release()
{
	if (data.size() > 0)
	{
		for (unsigned int i = 0; i < data.size(); i++) data[i].release();
		vector<OAMBank>().swap(data);
	}
	if (ext_data.size() > 0) vector<unsigned char>().swap(ext_data);
}

void NCERData::initial()
{
	sec_num = 1;
	flag = 0;
	ext_flag = 0;
	release();
}

void NCERData::initial(unsigned int num, unsigned int flg, unsigned int exf)
{
	sec_num = 1;
	flag = flg;
	ext_flag = exf;
	release();
	vector<OAMBank>(num).swap(data);
}

int NCERData::write(char *fileName)
{
	unsigned int tpa = 0;
	if (data.size() == 0) return -1;
	FILE *dst = fopen(fileName, "wb");
	if (dst == NULL) return -1;
	tpa = 0x4E434552;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x0100FEFF;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x30 + getDataSize() + ext_data.size();
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x10;
	fwrite(&tpa, 2, 1, dst);
	fwrite(&sec_num, 2, 1, dst);
	tpa = 0x4345424B;
	fwrite(&tpa, 4, 1, dst);
	tpa = 0x20 + getDataSize();
	fwrite(&tpa, 4, 1, dst);
	tpa = data.size();
	fwrite(&tpa, 2, 1, dst);
	fwrite(&ext_flag, 2, 1, dst);
	tpa = 0x18;
	fwrite(&tpa, 4, 1, dst);
	fwrite(&flag, 4, 1, dst);
	tpa = 0;
	fwrite(&tpa, 4, 1, dst);
	fwrite(&tpa, 4, 1, dst);
	fwrite(&tpa, 4, 1, dst);
	unsigned int oam_pos = 0;
	for (size_t i = 0; i < data.size(); i++)
	{
		tpa = data[i].data.size();
		fwrite(&tpa, 2, 1, dst);
		fwrite(&data[i].uk_flag, 2, 1, dst);
		fwrite(&oam_pos, 4, 1, dst);
		if (ext_flag == 1)
		{
			fwrite(&data[i].ext_data[0], 4, 1, dst);
			fwrite(&data[i].ext_data[1], 4, 1, dst);
		}
		oam_pos += data[i].data.size() * 6;
	}
	for (size_t i = 0; i < data.size(); i++) for (size_t j = 0; j < data[i].data.size(); j++) for (auto term : data[i].data[j].get()) fwrite(&term, 2, 1, dst);
	for (size_t i = 0; i < ext_data.size(); i++) putc(ext_data[i], dst);
	fclose(dst);
	return 0;
}

int NCERData::read(char *fileName)
{
	initial();
	unsigned int tpa = 0, tpb = 0;
	FILE *src = fopen(fileName, "rb");
	if (src == NULL) return -1;
	fseek(src, 0, 2);
	unsigned int fsize = ftell(src);
	rewind(src);
	if (fsize < 0x10)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	if (tpa != 0x4E434552)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	if (fsize < tpa)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 2, 1, src);
	fread(&sec_num, 2, 1, src);
	fread(&tpa, 4, 1, src);
	if (tpa != 0x4345424B)
	{
		fclose(src);
		return -1;
	}
	fread(&tpa, 4, 1, src);
	fread(&tpa, 2, 1, src);
	vector<OAMBank>(tpa).swap(data);
	fread(&ext_flag, 2, 1, src);
	fread(&tpa, 4, 1, src);
	if (tpa != 0x18)
	{
		fclose(src);
		return -1;
	}
	fread(&flag, 4, 1, src);
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	fread(&tpa, 4, 1, src);
	for (size_t i = 0; i < data.size(); i++)
	{
		fread(&tpa, 2, 1, src);
		fread(&tpb, 2, 1, src);
		data[i].initial(tpa, ext_flag, tpb);
		fread(&tpa, 4, 1, src);
		if (ext_flag == 1)
		{
			fread(&data[i].ext_data[0], 4, 1, src);
			fread(&data[i].ext_data[1], 4, 1, src);
		}
	}
	vector<unsigned short> attrs(3, 0);
	for (size_t i = 0; i < data.size(); i++) for (size_t j = 0; j < data[i].data.size(); j++)
	{
		fread(&attrs[0], 2, 1, src);
		fread(&attrs[1], 2, 1, src);
		fread(&attrs[2], 2, 1, src);
		data[i].set(j, attrs, flag);
	}
	while (ftell(src) < (long)fsize) ext_data.push_back(getc(src));
	return 0;
}

void NCERData::add(const OAMBank &bank)
{
	OAMBank rec(bank.data.size(), bank.ext_flag, bank.uk_flag, bank.x_min, bank.x_max, bank.y_min, bank.y_max);
	for (unsigned int i = 0; i < bank.data.size(); i++) rec.data[i] = bank.data[i];
	rec.width = bank.width;
	rec.height = bank.height;
	rec.x_pos = bank.x_pos;
	rec.y_pos = bank.y_pos;
	data.push_back(rec);
}

void NCERData::set(const OAMBank &bank, unsigned int id)
{
	if (id < data.size())
	{
		data[id].initial(bank.data.size(), bank.ext_flag, bank.uk_flag, bank.x_min, bank.x_max, bank.y_min, bank.y_max);
		for (unsigned int i = 0; i < bank.data.size(); i++) data[id].data[i] = bank.data[i];
		data[id].width = bank.width;
		data[id].height = bank.height;
		data[id].x_pos = bank.x_pos;
		data[id].y_pos = bank.y_pos;
	}
}

OAMBank NCERData::get(unsigned int id)
{
	OAMBank rec;
	if (id < data.size()) data[id].clone(rec);
	return rec;
}

unsigned int NCERData::getFlag()
{
	return flag;
}

unsigned int NCERData::getExtFlag()
{
	return ext_flag;
}

unsigned int NCERData::getSecNum()
{
	return sec_num;
}

unsigned int NCERData::getBankNum()
{
	return data.size();
}

unsigned int NCERData::getDataSize()
{
	unsigned int res = ext_flag == 1 ? data.size() * 0x10 : data.size() * 0x8;
	for (size_t i = 0; i < data.size(); i++) res += data[i].data.size() * 6;
	return res;
}
