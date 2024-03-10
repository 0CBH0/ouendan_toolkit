#pragma once

#include <imgConvLib.h>
#include <map>

class NCLRData
{
public:
	NCLRData();
	NCLRData(char *fileName);
	NCLRData(unsigned int type, unsigned int pn = 1, unsigned int sn = 1, unsigned int fga = 0, unsigned int fgb = 0);
	~NCLRData();

	void release();
	void initial();
	void initial(unsigned int type, unsigned int pn = 1, unsigned int sn = 1, unsigned int fga = 0, unsigned int fgb = 0);
	int write(char *fileName);
	int read(char *fileName);
	Color get(unsigned int pi, unsigned int ci) const;
	void set(Color col, unsigned int pi, unsigned int ci);
	int depthConv(unsigned int d);
	unsigned int getFlagA();
	unsigned int getFlagB();
	unsigned int getDepth();
	unsigned int getPalNum();
	unsigned int getColNum();
	unsigned int getSecNum();
	unsigned int getKey(Color c);
	Color getInvc(Color c);
	Color setAlpha();
	Color bg;
	vector<Color> bgRaw;
	vector<vector<Color>> data;

private:
	unsigned int depth;
	unsigned int sec_num;
	unsigned int flag_a;
	unsigned int flag_b;

};

class NCGRData
{
public:
	NCGRData();
	NCGRData(char *fileName);
	NCGRData(unsigned int type, unsigned int w, unsigned int h, unsigned int wb, unsigned int hb, unsigned int fg = 0, 
		unsigned int sn = 1, unsigned int cs = 0, unsigned int cc = 0);
	~NCGRData();

	void release();
	void initial();
	void initial(unsigned int type, unsigned int w, unsigned int h, unsigned int wb, unsigned int hb, unsigned int fg = 0, 
		unsigned int sn = 1, unsigned int cs = 0, unsigned int cc = 0);
	int write(char *fileName);
	int read(char *fileName);
	void add(vector<unsigned char> tile);
	vector<unsigned char> get(unsigned int id) const;
	void set(vector<unsigned char> tile, unsigned int id);
	void clone(NCGRData &dst) const;
	unsigned int getWidth();
	unsigned int getHeight();
	unsigned int getDepth();
	unsigned int getFlag();
	unsigned int getSecNum();
	unsigned int getTileNum();
	unsigned int getCS();
	unsigned int getCC();
	vector<vector<unsigned char>> data;
	unsigned int full;

private:
	unsigned int depth;
	unsigned int width;
	unsigned int height;
	unsigned int width_b;
	unsigned int height_b;
	unsigned int flag;
	unsigned int sec_num;
	unsigned int char_size;
	unsigned int char_count;

};

class NSCRData
{
public:
	NSCRData();
	NSCRData(char *fileName);
	NSCRData(unsigned int w, unsigned int h, unsigned int is = 0, unsigned int bt = 0);
	~NSCRData();

	void release();
	void initial();
	void initial(unsigned int w, unsigned int h, unsigned int is = 0, unsigned int bt = 0);
	int write(char *fileName);
	int read(char *fileName);
	int modify(char *fileName, NCGRData &ncgr, NCLRData &nclr, bool keepTile = 1);
	unsigned short get(unsigned int id) const;
	unsigned int getFlip(unsigned int id) const;
	unsigned int getTileId(unsigned int id) const;
	unsigned int getPalId(unsigned int id) const;
	void set(unsigned short info, unsigned int id);
	unsigned int getWidth();
	unsigned int getHeight();
	unsigned int getSize();
	unsigned int getISSize();
	unsigned int getBGType();
	unsigned int getInfoNum();
	vector<unsigned short> data;

private:
	unsigned int width;
	unsigned int height;
	unsigned int width_raw;
	unsigned int height_raw;
	unsigned int is_size;
	unsigned int bg_type;

};

class OAMData
{
public:
	OAMData();
	OAMData(const vector<unsigned short> &attrs, unsigned int flags = 0, int lay = -1);

	OAMData operator = (const OAMData &src);

	void initial();
	void set(const vector<unsigned short> &attrs, unsigned int flags = 0, int lay = -1);
	vector<unsigned short> get() const;
	vector<unsigned short> toMap(unsigned int tileCol = 1);
	int calcWidth();
	int calcHeight();
	OAMData clone() const;

	int xpos;
	int ypos;
	int width;
	int height;
	int layer;
	unsigned char aa_flag;
	unsigned char ab_flag;
	unsigned char ac_flag;
	unsigned char ad_flag;
	unsigned char ds_flag;
	unsigned char rs_flag;
	unsigned char depth;
	unsigned short shape;
	unsigned short size;
	unsigned short hv_mode;
	unsigned int tile_index;
	unsigned int pal_index;
	unsigned int shift;

private:

};

class OAMBank
{
public:
	OAMBank();
	OAMBank(unsigned int num, unsigned int exf = 0, unsigned int ukf = 0x08, short xs = 0, short xl = 0, short ys = 0, short yl = 0);
	~OAMBank();

	void release();
	void initial();
	void initial(unsigned int num, unsigned int exf = 0, unsigned int ukf = 0x08, short xs = 0, short xl = 0, short ys = 0, short yl = 0);
	void calcSize();
	void resetImg();
	void add(const vector<unsigned short> &attrs, unsigned int flags = 0, unsigned int lay = 0);
	void add(const OAMData &term);
	void set(unsigned int id, const vector<unsigned short> &attrs, unsigned int flags = 0, unsigned int lay = 0);
	void set(unsigned int id, const OAMData &term);
	OAMData get(unsigned int id);
	void clone(OAMBank &dst) const;
	int draw(unsigned int id, NCGRData &ncgr, NCLRData &nclr, bool cover = 0);
	int modify(int layer, vector<OAMData> &oamList, char *fileName, NCGRData &ncgr, NCLRData &nclr, bool keepTile = 1);
	void write(char *fileName);

	unsigned int ext_flag;
	unsigned int uk_flag;
	unsigned int ext_data[2];
	int width;
	int height;
	int x_pos;
	int y_pos;
	short x_min;
	short x_max;
	short y_min;
	short y_max;
	Color bg;
	vector<OAMData> data;
	ImageData img;

private:
};

class NCERData
{
public:
	NCERData();
	NCERData(char *fileName);
	NCERData(unsigned int num, unsigned int flg = 0, unsigned int exf = 0);
	~NCERData();

	void release();
	void initial();
	void initial(unsigned int num, unsigned int flg = 0, unsigned int exf = 0);
	int write(char *fileName);
	int read(char *fileName);
	void add(const OAMBank &bank);
	void set(const OAMBank &bank, unsigned int id);
	OAMBank get(unsigned int id);
	unsigned int getFlag();
	unsigned int getExtFlag();
	unsigned int getSecNum();
	unsigned int getBankNum();
	unsigned int getDataSize();
	vector<OAMBank> data;
	vector<unsigned char> ext_data;

private:
	unsigned int sec_num;
	unsigned int flag;
	unsigned int ext_flag;

};

