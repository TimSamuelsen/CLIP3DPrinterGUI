/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

#ifndef PTN_IMAGE_H
#define PTN_IMAGE_H

#include "pattern.h"
#include "splash.h"
#include "qimage.h"
#include "qpixmap.h"
#include <QImageWriter>
/**
 * Class for loading, storing and manipulating the pattern image.
 * This class can be interchangeably used with QImage and hence extend the
 * capability of QImage.
 */

class PtnImage
{
private:
    Image_t *img;  /**< Pattern image data structure */
    QImage qimg24; /**< QImage data structure */
    uint08 *splashData; /**< Splash data of the image */
    bool freeImg;



public:
    /**
     * Construct the pattern image from Image_t structure. The Image_t structure
     * should not be freed by the caller.
     *
     * @param image Image_t data structure
     *
     */
    PtnImage(Image_t *image)
    {
        splashData = NULL;
        freeImg = false;
        img = image;
        if(img->Format == PTN_RGB24)
        {
            qimg24 = QImage(img->Buffer, img->Width, img->Height,
                            img->LineWidth, QImage::Format_RGB888);
        }
        else if(img->Format == PTN_GRAYSCALE16)
        {
            qimg24 = QImage(img->Buffer, img->Width, img->Height,
                            img->LineWidth, QImage::Format_Grayscale16);
        }
    }

	/**
	 * Construct the pattern image from an image file. The dimension of the 
	 * pattern image will be same as the original image.
	 *
	 * @param fileName - Image file to be loaded
	 */
    PtnImage(QString fileName)
    {
        QImage qimg(fileName);
#if IMG_DEBUG
        QImageWriter writer("D:/imagepostconversion.png","PNG");
#endif
        if(qimg.format() <= QImage::Format_RGBA8888_Premultiplied)
        {
            if(qimg.format() != QImage::Format_RGB888)
            {
                qimg24 = qimg.convertToFormat(QImage::Format_RGB888);
            }
            else
            {
                qimg24 = qimg;
            }
            img = PTN_Alloc(0,0,0,PTN_RGB24);
            img->Format = PTN_RGB24;
            img->BitDepth = 24;
        }
        else
        {
            if(qimg.format() != QImage::Format_Grayscale16)
            {
                qimg24 = qimg.convertToFormat(QImage::Format_Grayscale16);
            }
            else
            {
                qimg24 = qimg;
            }
            img = PTN_Alloc(0,0,0,PTN_GRAYSCALE16);
            img->Format = PTN_GRAYSCALE16;
            img->BitDepth = 16;
        }
#if IMG_DEBUG
        writer.write(qimg24);
#endif
        img->Width = qimg24.width();
        img->Height = qimg24.height();
        img->LineWidth = qimg24.scanLine(1) - qimg24.scanLine(0);
        img->Buffer = qimg24.scanLine(0);
        img->FullHeight = qimg24.height();
        splashData = NULL;
        freeImg = true;
    }

	/**
	 * Construct an empty pattern image with given width and height.
	 * Content of the image will be garbage. Need to either load, copy or
	 * fill the image before using.
	 *
	 * @param width - Width of the pattern image
	 * @param height - Height of the pattern image
	 * @param bitDepth - Bit depth of the pattern image
     * @param format - Format of the pattern image
	 */
    PtnImage(int width, int height, int bitDepth, PTN_Format_t format)
    {
        img = PTN_Alloc(width, height, bitDepth, format);
        if(format == PTN_RGB24)
        {
            qimg24 = QImage(img->Buffer, img->Width, img->Height,
                            img->LineWidth, QImage::Format_RGB888);
        }
        else if (format == PTN_GRAYSCALE16)
        {
            qimg24 = QImage(img->Buffer, img->Width, img->Height,
                            img->LineWidth, QImage::Format_Grayscale16);
        }
        splashData = NULL;
        freeImg = true;
    }

	/**
	 * Construct a pattern image from given QImage. The dimension of the 
	 * pattern image will be same as the QImage.
	 * @param qimg - Source QImage
	 */
    PtnImage(QImage const &qimg)
    {
        if(qimg.format() <= QImage::Format_RGBA8888_Premultiplied)
        {
            if(qimg.format() != QImage::Format_RGB888)
            {
                qimg24 = qimg.convertToFormat(QImage::Format_RGB888);
            }
            else
            {
                qimg24 = qimg;
            }
            img->Format = PTN_RGB24;
            img->BitDepth = 24;
        }
        else
        {
            if(qimg.format() != QImage::Format_Grayscale16)
            {
                qimg24 = qimg.convertToFormat(QImage::Format_Grayscale16);
            }
            else
            {
                qimg24 = qimg;
            }
            img->Format = PTN_GRAYSCALE16;
            img->BitDepth = 16;
        }
        img = PTN_Alloc(0,0,0,img->Format);
        img->Width = qimg24.width();
        img->Height = qimg24.height();
        img->LineWidth = qimg24.scanLine(1) - qimg24.scanLine(0);
        img->Buffer = qimg24.scanLine(0);
        img->FullHeight = qimg24.height();
        freeImg = true;
    }

	/**
	 * Construct a pattern image from another pattern image
	 *
	 * @param ptn Source pattern image
	 *
	 */
    PtnImage(PtnImage const &ptn)
    {
        img = PTN_Alloc(ptn.img->Width, ptn.img->Height, ptn.img->BitDepth, ptn.img->Format);
        qimg24 = QImage(img->Buffer, img->Width, img->Height,
                            img->LineWidth, QImage::Format_RGB888);
        freeImg = true;
    }

	/**
	 * Assignment from pattern image to pattern image
	 */
    void operator = (PtnImage const &ptn)
    {
        PTN_Copy(img, ptn.img);
    }

	/**
	 * Assignment from QImage to pattern image
	 */
    void operator = (QImage const &qimg)
    {
        PtnImage ptn(qimg);
        PTN_Copy(img, ptn.img);
    }

	/**
	 * Converts this pattern image to a QImage
	 */
    operator QImage()
    {
        return qimg24;
    }

	/**
	 * Converts this pattern image to a QPixmap
	 */
    operator QPixmap()
    {
        return QPixmap::fromImage(qimg24);
    }

	/**
	 * Load an image file to this pattern image. The image will be accordingly
	 * cropped based on this image dimension and quantized based on the bitdepth
	 * of this image
	 * @param fileName - Image file name
	 *
	 * @return 0 On success negative otherwise
	 */
    int loadFromFile(QString fileName)
    {
        PtnImage fileImg(fileName);
        return PTN_Copy(img, fileImg.img);
    }

	/**
	 * Loads a compressed/uncompressed splash image to this image. Bit depth will
	 * be set to full color (24bit)
	 *
	 * @param splash - Splash image byte stream
	 *
	 * @return 0 On success negative otherwise
	 */
    int loadSplash(uint08 const *splash)
    {
        return SPL_ConvSplashToImage(splash, img);
    }

	/**
     * Converts this pattern image to splash image format.
     * Note: Splash compression not applicable for PTN_Format PTN_Grayscale16
	 * @param size - Number of bytes in the splash image
	 *
	 * @return pointer to the converted splash image
	 */
    int toSplash(unsigned char **data,SPL_Compression_t compression)
    {
        if(splashData == NULL)
        {
            splashData = SPL_AllocSplash(img->Width, img->Height);
            if(splashData == NULL)
                return ERR_OUT_OF_RESOURCE;
        }

        *data = splashData;

        return SPL_ConvImageToSplash(img, compression, splashData);
    }

	/**
	 * Merges the bits from the source pattern to this pattern image
	 * @param ptn Source pattern image
	 * @param bitPos Destination bit position to be merged to
	 * @return 0 on success, negative otherwise
	 */
    int merge(PtnImage const &ptn, int bitPos, int bitDepth)
    {
        return PTN_Merge(img, ptn.img,bitPos, bitDepth);
    }

	/**
	 * Quantizes this image to the given bitdepth
	 *
	 * @param bitDepth - Bitdepth to be quantized to (1 - 8)
	 *
	 * @return 0 on success, -ve otherwise
	 */
    int quantize(int bitDepth)
    {
        return PTN_Quantize(img, img, bitDepth);
    }

	/**
	 * Quantize the given pattern image and copy it to this pattern image
	 *
	 * @param ptn Source pattern image to be quantized
	 * @param bitDepth Bit depth to be quantized to
	 *
	 * @return 0 on success, -ve otherwise
	 */
    int quantize(PtnImage const &ptn, int bitDepth = 0)
    {
        return PTN_Quantize(img, ptn.img, bitDepth);
    }

	/**
	 * Extracts the specific bits of the given image pixels and copies it to 
	 * this pattern image.
	 *
	 * @param ptn - Source image
	 * @param bitPos - Bit position from where to extract the bits (0 -31)
	 * @param bitDepth - Number of bits to extract (1 - 8). If 0 then
	 *        the current pattern image bit depth is used
	 *
	 * @return 0 on success, -ve otherwise
	 */
    int extract(PtnImage const &ptn, int bitPos, int bitDepth = 0)
    {
        return PTN_Extract(img, ptn.img, bitPos, bitDepth);
    }

	/**
	 * Extracts the specific bits of this image to make the final image
	 *
	 * @param bitPos - Bit position from where to extract the bits (0 -31)
	 * @param bitDepth - Number of bits to extract (1 - 8)
	 *
	 * @return 0 on success, -ve otherwise
	 */
    PtnImage extract(int bitPos, int bitDepth)
    {
        PtnImage ext(*this);
        PTN_Extract(ext.img, img, bitPos, bitDepth);
        return ext;
    }

    /**
     * Crops this image for the given dimension
     * @param x Start x
     * @param y Start y
     * @param width New width
     * @param height new height
     *
     * @return 0 on success, -1 on failure
     */
    int crop(int x, int y, int width, int height)
    {
        return PTN_Crop(img, x, y, width, height);
    }

	/**
	 * Fills the full image with given byte pattern
	 *
	 * @param value byte pattern to be filled
	 *
	 * @return 0 on success, -ve otherwise
	 */
    int fill(int value)
    {
        return PTN_Fill(img, value);
    }

    /**
     * Swaps the colors channels according to the user defined position
     *
     * @param red Red color should change to this
     * @param green Green color should change to this
     * @param blue Blue color should change to this
     *
     * @return 0 on success, -ve otherwise
     */
    int swapColors(PTN_Color_t red, PTN_Color_t green, PTN_Color_t blue)
    {
        return PTN_SwapColors(img, red, green, blue);
    }

    /**
     * Returns the Image_t version of the pattern image
     * @return pointer to the Image_t structure
     */
    Image_t *toImage(void)
    {
        freeImg = false;
        return img;
    }

	/**
	 * Destructor for the pattern image
	 */
    ~PtnImage()
    {
        if(freeImg)
            PTN_Free(img);
        SPL_Free(splashData);
    }

	/**
	 * Convert the pattern image to QImage
	 *
	 * @return QImage equlant of the pattern image
	 */
    QImage const &qimage(void)
    {
        return qimg24;
    }

	/**
	 * Gets the width of the image
	 *
	 * @return Width of the image in pixels
	 */
	int width(void)
	{
		return img->Width;
	}

	/**
	 * Gets the height of the image
	 *
	 * @return Height of the image in pixels
	 */
	int height(void)
	{
		return img->Height;
	}
};


#endif // PATTERN_H
