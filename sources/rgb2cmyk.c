#define MaxRGB 255
cyan=MaxRGB-p->red;
magenta=MaxRGB-p->green;
yellow=MaxRGB-p->blue;
black=cyan;
if (magenta < black)
black=magenta;
if (yellow < black)
black=yellow;
                                                                    