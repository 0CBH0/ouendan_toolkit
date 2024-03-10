#include "ncxr.h"

#pragma warning(disable : 4996)

int imgOut(char *imgFileName, char *nscrFileName, char *ncgrFileName, char *nclrFileName);
int imgIn(char *imgFileName, char *nscrFileName, char *ncgrFileName, char *nclrFileName, int times);

int main(int argc, char* argv[])
{
	if (argc < 6)
	{
		printf("usage:\n");
		printf("yyt2_nscr out <imgae_file> <nscr_file> <ncgr_file> <nclr_file>\n");
		printf("yyt2_nscr in <imgae_file> <nscr_file> <ncgr_file> <nclr_file> [times]\n");
	}
	else if (!strcmp("out", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		if (!imgOut(argv[2], argv[3], argv[4], argv[5])) printf("Done!\n"); else printf("Fail!\n");
	}
	else if (!strcmp("in", argv[1]))
	{
		printf("Processing %s ... ", argv[3]);
		int times = 0;
		if (argc == 7) times = atoi(argv[6]);
		if (!imgIn(argv[2], argv[3], argv[4], argv[5], times)) printf("Done!\n"); else printf("Fail!\n");
	}
	return 0;
}

int imgOut(char *imgFileName, char *nscrFileName, char *ncgrFileName, char *nclrFileName)
{
	NCLRData nclr(nclrFileName);
	if (nclr.getPalNum() == 0) return -1;
	NCGRData ncgr(ncgrFileName);
	if (ncgr.getTileNum() == 0) return -1;
	if (ncgr.getDepth() != nclr.getDepth() && nclr.depthConv(ncgr.getDepth()) == -1) return -1;
	Color bg = nclr.setAlpha();
	NSCRData nscr(nscrFileName);
	if (nscr.getInfoNum() == 0) return -1;
	ImageData img;
	if (TMPToImage(img, ncgr.data, nscr.data, nclr.data, nscr.getWidth(), nscr.getHeight(), 0, ncgr.getDepth() - 3, 1) >= 0) img.write(imgFileName);
	nclr.release();
	ncgr.release();
	nscr.release();
	return 0;
}

int imgIn(char *imgFileName, char *nscrFileName, char *ncgrFileName, char *nclrFileName, int times)
{
	NCLRData nclr(nclrFileName);
	if (nclr.getPalNum() == 0) return -1;
	NCGRData ncgr(ncgrFileName);
	if (ncgr.getTileNum() == 0) return -1;
	if (ncgr.getDepth() != nclr.getDepth() && nclr.depthConv(ncgr.getDepth()) == -1) return -1;
	Color bg = nclr.setAlpha();
	NSCRData nscr(nscrFileName);
	if (nscr.getInfoNum() == 0) return -1;
	if (nscr.getBGType() != 0) return -1;
	if (times > 0)
	{
		ImageData img(imgFileName);
		if (img.size == nscr.getSize())
		{
			if (ImageToTMP(img, ncgr.data, nscr.data, nclr.data, bg, 0, nclr.getDepth() - 3, nclr.getPalNum(), nclr.getColNum(), times) != -1 && ncgr.full == 0)
			{
				nclr.write(nclrFileName);
				ncgr.write(ncgrFileName);
				nscr.write(nscrFileName);
			}
		}
		img.release();
	}
	else
	{
		bool keepTile = times == 0 ? 0 : 1;
		if (nscr.modify(imgFileName, ncgr, nclr, keepTile) != -1 && ncgr.full == 0)
		{
			ncgr.write(ncgrFileName);
			nscr.write(nscrFileName);
		}
	}
	nclr.release();
	ncgr.release();
	nscr.release();
	return 0;
}
