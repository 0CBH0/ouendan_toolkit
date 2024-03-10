# ouendan_toolkit
The toolkit used for image handle of ouendan series

- yyt2_decode: decompress the file compressed by lz10/lzss, compress file using lzss
- yyt2_ncer: export/import the images from *.ncer files
- yyt2_nscr: export/import the image from *.nscr files
- yyt2_ntft: export/import the image from *.ntft files

# Usage
~~~
# decompress/compress (auto.):
yyt2_decode <file_name>

# export/import images from *.ncer:
yyt2_ncer out <dir_image> <file_ncer> <file_ncgr> <file_nclr>
yyt2_ncer in <dir_image> <file_ncer> <file_ncgr> <file_nclr>

# export/import images from *.nscr:
yyt2_nscr out <file_image> <file_nscr> <file_ncgr> <file_nclr>
yyt2_nscr in <file_image> <file_nscr> <file_ncgr> <file_nclr>

# export/import images from *.ntft:
yyt2_nscr out <file_image> <file_ntft> <file_ntfp> <width>
yyt2_nscr in <file_image> <file_ntft> <file_ntfp> <width>
~~~

# Dependencies
imgConvLib
libimagequant

