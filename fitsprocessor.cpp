/*
    MIT License

    Copyright (c) 2021 Astrocat.App

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "fitsprocessor.h"
#include "fitsio.h"

FitsProcessor::FitsProcessor(QObject *parent) : QObject(parent)
{

}

QImage makeThumbnail(const QImage &image)
{
    QImage small =image.scaled( QSize(200, 200), Qt::KeepAspectRatio);
    return small;
}


void FitsProcessor::cancel()
{
    cancelSignaled = true;
}

QMap<QString, QString> GetTags(const AstroFileImage& astroFileImage)
{
    fitsfile *fptr;
    int status = 0;
    int hdupos, nkeys;
    char card[FLEN_CARD];
    QMap<QString, QString> tagsMap;

    QString fullPath = astroFileImage.astroFile.FullPath;

    if (!fits_open_file(&fptr, fullPath.toStdString().c_str(), READONLY, &status))
    {
        fits_get_hdu_num(fptr, &hdupos);

        while (!status)
        {
            hdupos++;
            fits_get_hdrspace(fptr, &nkeys, NULL, &status);

            for (int i = 1; i <= nkeys; i++)
            {
               if (fits_read_record(fptr, i, card, &status))
                   break;

               char keyname[FLEN_KEYWORD];
               char keyvalue[FLEN_VALUE];
               char comment[FLEN_COMMENT];

               fits_read_keyn(fptr,i, keyname, keyvalue, comment, &status);
               tagsMap.insert(QString(keyname).remove("'").trimmed(), QString(keyvalue).remove("'").trimmed());
            }

            fits_movrel_hdu(fptr, 1, NULL, &status);
        }
    }
    return tagsMap;
}

QImage getPixels(const AstroFileImage& astroFileImage)
{
    fitsfile *fptr;
    int status = 0;
    int bitpix, naxis;
    long naxes[2] = {1,1};

    QImage img;
    QString fullPath = astroFileImage.astroFile.FullPath;

    if (!fits_open_file(&fptr, fullPath.toStdString().c_str(), READONLY, &status))
    {
        if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
          if (naxis > 2 || naxis == 0)
             qDebug() << "Error: only 1D or 2D images are supported";
          else
          {
              std::vector<uint8_t> bits8;
              std::vector<uint16_t> bits16;
              std::vector<uint32_t> bits32;
              long fpixel[3] = {1,1,1};

              size_t size = naxes[0]*naxes[1];
              size_t w = naxes[0];
              size_t h = naxes[1];

              switch (bitpix)
              {
              case BYTE_IMG:
                  break;
              case SHORT_IMG:
                  bits16.resize(size);
                  fits_read_pix(fptr, TUSHORT, fpixel, size, NULL, &bits16[0], NULL, &status);
                  if(status)
                  {
                      char err_text[1024];
                      fits_get_errstatus( status, err_text);

                      break;
                  }

                  img = QImage(w, h, QImage::Format_Grayscale8);
                  for(size_t i=0; i<h; i++)
                  {
                      uchar* ptr = img.scanLine(i);
                      for(size_t o=0;o<w;o++)
                      {
                          ptr[o] = bits16[i*w+o] >> 8;
                      }
                  }
                  if (img.isNull())
                  {
                      qDebug() << "Null Image";
                      bool ret = img.load("nopreview.png");
                      if (!ret)
                          qDebug() << "Failed to load nopreview.png";
                  }

                  break;
              case LONG_IMG:
                  break;
              case LONGLONG_IMG:
                  break;
              case FLOAT_IMG:
                  break;
              case DOUBLE_IMG:
                  break;
              }

            AstroFileImage f(astroFileImage);
          }
        }
        fits_close_file(fptr, &status);
    }
    return img;
}

void FitsProcessor::extractTags(const AstroFileImage &astroFileImage)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining Fits Tag Queue.";
        return;
    }

    auto tags = GetTags(astroFileImage);
    emit tagsExtracted(astroFileImage, tags);
}

void FitsProcessor::extractThumbnail(const AstroFileImage &astroFileImage)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining Fits Thumbnail Queue.";
        return;
    }

    auto image = getPixels(astroFileImage);
    auto thumbnail = makeThumbnail(image);
    emit thumbnailExtracted(astroFileImage, thumbnail);
}

