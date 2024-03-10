#include "../yyt2_nscr/ncxr.h"
#include <direct.h>

#pragma warning(disable : 4996)

int imgOut(char *imgDirName, char *ncerFileName, char *ncgrFileName, char *nclrFileName);
int imgIn(char *imgDirName, char *ncerFileName, char *ncgrFileName, char *nclrFileName, int times);

int main(int argc, char* argv[])
{
	if (argc < 6)
	{
		printf("usage:\n");
		printf("yyt2_ncer out <imgae_dir> <ncer_file> <ncgr_file> <nclr_file>\n");
		printf("yyt2_ncer in <imgae_dir> <ncer_file> <ncgr_file> <nclr_file> [times]\n");
	}
	else if (!strcmp("out", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		if (!imgOut(argv[2], argv[3], argv[4], argv[5])) printf("Done!\n"); else printf("Fail!\n");
	}
	else if (!strcmp("in", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		unsigned int times = 0;
		if (argc == 7) times = atoi(argv[6]);
		if (!imgIn(argv[2], argv[3], argv[4], argv[5], times)) printf("Done!\n"); else printf("Fail!\n");
	}
	return 0;
}

int imgOut(char *imgDirName, char *ncerFileName, char *ncgrFileName, char *nclrFileName)
{
	if (strlen(imgDirName) == 0) return -1;
	NCLRData nclr(nclrFileName);
	if (nclr.getPalNum() == 0) return -1;
	NCGRData ncgr(ncgrFileName);
	if (ncgr.getTileNum() == 0) return -1;
	if (ncgr.getDepth() != nclr.getDepth() && nclr.depthConv(ncgr.getDepth()) == -1) return -1;
	Color bg = nclr.setAlpha();
	NCERData ncer(ncerFileName);
	if (ncer.getBankNum() == 0) return -1;
	//if ((ncer.getFlag() & 3) > 1) printf("sp: %d ", ncer.getFlag() & 3);
	if (imgDirName[strlen(imgDirName) - 1] == '\\' || imgDirName[strlen(imgDirName) - 1] == '/') imgDirName[strlen(imgDirName) - 1] = '\0';
	mkdir(imgDirName);
	char imgName[260];
	unsigned int imgNamePosA = 0, imgNamePosB = strlen(ncerFileName);
	for (unsigned int i = strlen(ncerFileName) - 1; i > 0; i--) if (ncerFileName[i] == '\\' || ncerFileName[i] == '/') { imgNamePosA = i + 1; break; }
	for (unsigned int i = strlen(ncerFileName) - 1; i > 0; i--) if (ncerFileName[i] == '.') { imgNamePosB = i; break; }
	for (unsigned int i = 0; i < imgNamePosB - imgNamePosA; i++) imgName[i] = ncerFileName[i + imgNamePosA];
	imgName[imgNamePosB - imgNamePosA] = '\0';
	char imgFileName[260];
	for (size_t i = 0; i < ncer.data.size(); i++)
	{
		int layer = 0;
		ncer.data[i].bg = bg;
		ncer.data[i].calcSize();
		ncer.data[i].resetImg();
		//printf("\nBANK%d: %d,%d,%d,%d:\n", i, ncer.data[i].width, ncer.data[i].height, ncer.data[i].x_pos, ncer.data[i].y_pos);
		for (size_t j = 0; j < ncer.data[i].data.size(); j++)
		{
			//printf("OAM%d: %d,%d,%d,%d,%d\n", j, ncer.data[i].data[j].xpos, ncer.data[i].data[j].ypos, ncer.data[i].data[j].width, ncer.data[i].data[j].height, ncer.data[i].data[j].tile_index);
			if (ncer.data[i].draw(j, ncgr, nclr) == 1)
			{
				sprintf(imgFileName, "%s/%s_%d_%d.bmp", imgDirName, imgName, i, layer);
				ncer.data[i].write(imgFileName);
				ncer.data[i].resetImg();
				ncer.data[i].draw(j, ncgr, nclr);
				layer++;
			}
			ncer.data[i].data[j].layer = layer;
		}
		sprintf(imgFileName, "%s/%s_%d_%d.bmp", imgDirName, imgName, i, layer);
		ncer.data[i].write(imgFileName);
	}
	nclr.release();
	ncgr.release();
	ncer.release();
	return 0;
}

int imgIn(char *imgDirName, char *ncerFileName, char *ncgrFileName, char *nclrFileName, int times)
{
	if (strlen(imgDirName) == 0) return -1;
	NCLRData nclr(nclrFileName);
	if (nclr.getPalNum() == 0) return -1;
	NCGRData ncgr(ncgrFileName);
	if (ncgr.getTileNum() == 0) return -1;
	if (ncgr.getDepth() != nclr.getDepth() && nclr.depthConv(ncgr.getDepth()) == -1) return -1;
	Color bg = nclr.setAlpha();
	NCERData ncer(ncerFileName);
	if (ncer.getBankNum() == 0) return -1;
	NCGRData ncgrMod;
	ncgr.clone(ncgrMod);
	bool keepTile = times >= 0 ? 0 : 1;
	if (!keepTile)
	{
		for (size_t i = 0; i < ncgrMod.data.size(); i++) vector<unsigned char>().swap(ncgrMod.data[i]);
		vector<vector<unsigned char>>().swap(ncgrMod.data);
	}
	if (imgDirName[strlen(imgDirName) - 1] == '\\' || imgDirName[strlen(imgDirName) - 1] == '/') imgDirName[strlen(imgDirName) - 1] = '\0';
	mkdir(imgDirName);
	char imgName[260];
	unsigned int imgNamePosA = 0, imgNamePosB = strlen(ncerFileName);
	for (unsigned int i = strlen(ncerFileName) - 1; i > 0; i--) if (ncerFileName[i] == '\\' || ncerFileName[i] == '/') { imgNamePosA = i + 1; break; }
	for (unsigned int i = strlen(ncerFileName) - 1; i > 0; i--) if (ncerFileName[i] == '.') { imgNamePosB = i; break; }
	for (unsigned int i = 0; i < imgNamePosB - imgNamePosA; i++) imgName[i] = ncerFileName[i + imgNamePosA];
	imgName[imgNamePosB - imgNamePosA] = '\0';
	char imgFileName[260];
	bool rec = false;
	for (size_t i = 0; i < ncer.data.size(); i++)
	{
		int layer = 0;
		vector<OAMData> oamList;
		ncer.data[i].bg = bg;
		ncer.data[i].calcSize();
		ncer.data[i].resetImg();
		//printf("\nBANK%d: %d,%d,%d,%d:\n", i, ncer.data[i].width, ncer.data[i].height, ncer.data[i].x_pos, ncer.data[i].y_pos);
		for (size_t j = 0; j < ncer.data[i].data.size(); j++)
		{
			//printf("OAM%d: %d,%d,%d,%d,%d\n", j, ncer.data[i].data[j].xpos,ncer.data[i].data[j].ypos, ncer.data[i].data[j].width, ncer.data[i].data[j].height, ncer.data[i].data[j].tile_index);
			if (ncer.data[i].draw(j, ncgr, nclr) == 1)
			{
				sprintf(imgFileName, "%s/%s_%d_%d.bmp", imgDirName, imgName, i, layer);
				if (ncer.data[i].modify(layer, oamList, imgFileName, ncgrMod, nclr, keepTile) >= 0) rec = true;
				ncer.data[i].resetImg();
				ncer.data[i].draw(j, ncgr, nclr);
				layer++;
			}
			ncer.data[i].data[j].layer = layer;
		}
		sprintf(imgFileName, "%s/%s_%d_%d.bmp", imgDirName, imgName, i, layer);
		if (ncer.data[i].modify(layer, oamList, imgFileName, ncgrMod, nclr, keepTile) >= 0) rec = true;
		oamList.swap(ncer.data[i].data);
		vector<OAMData>().swap(oamList);
	}
	if (rec && ncgrMod.full == 0)
	{
		ncgrMod.write(ncgrFileName);
		ncer.write(ncerFileName);
	}
	nclr.release();
	ncgr.release();
	ncgrMod.release();
	ncer.release();
	return 0;
}
