/*
 * Program to transfer colors from one image to another
 * 
 * Command line parameters are as follows:
 *
 * colortransfer source.png destination.png [outfile.png]
 *
 * Author: Drake Hunter, 12/2/2019
 * Credits: Ioannis Karamouzas, 10/20/19
 */

#include "matrix.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <OpenImageIO/imageio.h>
#include <GL/glut.h>

using namespace std;
OIIO_NAMESPACE_USING


struct Pixel { // defines a pixel structure
  unsigned char r,g,b,a;
}; 

using std::string;

//
// Global variables and constants
//
const int DEFAULTWIDTH = 600; // default window dimensions if no image
const int DEFAULTHEIGHT = 600;

int WinWidth, WinHeight;  // window width and height
int DestImWidth, DestImHeight;    // dest image width and height
int SourceImWidth, SourceImHeight;    // source image width and height
int H2, W2;
int ImChannels;           // number of channels per image pixel
int SourceImChannels;

int VpWidth, VpHeight;    // viewport width and height
int Xoffset, Yoffset;     // viewport offset from lower left corner of window

Pixel **source = NULL;  // the image pixmap used for reading in
Pixel **scaledsource = NULL;
Pixel **dest = NULL;
Pixel **display = NULL; // the image pixmap used for display
Pixel **out = NULL; // the image pixmap used for output

int pixformat;      // the pixel format used to correctly  draw the image

void destroy(){
  if(dest) {
      delete dest[0];
    delete dest;  
  }
}

void destroysource(){
  if(source) {
      delete source[0];
    delete source;  
  }
}

//
//  Routine to read an image file and store in a dest
//  returns the size of the image in pixels if correctly read, or 0 if failure
//
int readimage(string infilename){
  // Create the oiio file handler for the image, and open the file for reading the image.
  // Once open, the file spec will indicate the width, height and number of channels.
  ImageInput *infile = ImageInput::open(infilename);
  if(!infile){
    cerr << "Could not input image file " << infilename << ", error = " << geterror() << endl;
    return 0;
  }

  // Record image width, height and number of channels in global variables
  DestImWidth = infile->spec().width;
  DestImHeight = infile->spec().height;
  ImChannels = infile->spec().nchannels;

 
  // allocate temporary structure to read the image 
  unsigned char tmp_pixels[DestImWidth * DestImHeight * ImChannels];

  // read the image into the tmp_pixels from the input file, flipping it upside down using negative y-stride,
  // since OpenGL pixmaps have the bottom scanline first, and 
  // oiio expects the top scanline first in the image file.
  int scanlinesize = DestImWidth * ImChannels * sizeof(unsigned char);
  if(!infile->read_image(TypeDesc::UINT8, &tmp_pixels[0] + (DestImHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not read image from " << infilename << ", error = " << geterror() << endl;
    ImageInput::destroy(infile);
    return 0;
  }
 
 // get rid of the old OpenGL dest and make a new one of the new size
  destroy();
  
 // allocate space for the dest (contiguous approach)
  dest = new Pixel*[DestImHeight];
  if(dest != NULL)
  dest[0] = new Pixel[DestImWidth * DestImHeight];
  for(int i = 1; i < DestImHeight; i++)
  dest[i] = dest[i - 1] + DestImWidth;
 
 //  assign the read pixels to the dest
 int index;
  for(int row = 0; row < DestImHeight; ++row) {
    for(int col = 0; col < DestImWidth; ++col) {
    index = (row*DestImWidth+col)*ImChannels;
    
    if (ImChannels==1){ 
      dest[row][col].r = tmp_pixels[index];
      dest[row][col].g = tmp_pixels[index];
      dest[row][col].b = tmp_pixels[index];
      dest[row][col].a = 255;
    }
    else{
      dest[row][col].r = tmp_pixels[index];
      dest[row][col].g = tmp_pixels[index+1];
      dest[row][col].b = tmp_pixels[index+2];     
      if (ImChannels <4) // no alpha value is present so set it to 255
        dest[row][col].a = 255; 
      else // read the alpha value
        dest[row][col].a = tmp_pixels[index+3];     
    }
    }
  }
 
  // close the image file after reading, and free up space for the oiio file handler
  infile->close();
  ImageInput::destroy(infile);
  
  // set the pixel format to GL_RGBA and fix the # channels to 4  
  pixformat = GL_RGBA;  
  ImChannels = 4;

  // return image size in pixels
  return DestImWidth * DestImHeight;
}

int readsourceimage(string infilename){
  // Create the oiio file handler for the image, and open the file for reading the image.
  // Once open, the file spec will indicate the width, height and number of channels.
  ImageInput *infile = ImageInput::open(infilename);
  if(!infile){
    cerr << "Could not input image file " << infilename << ", error = " << geterror() << endl;
    return 0;
  }

  // Record image width, height and number of channels in global variables
  SourceImWidth = infile->spec().width;
  SourceImHeight = infile->spec().height;
  SourceImChannels = infile->spec().nchannels;

 
  // allocate temporary structure to read the image 
  unsigned char tmp_pixels[SourceImWidth * SourceImHeight * SourceImChannels];

  // read the image into the tmp_pixels from the input file, flipping it upside down using negative y-stride,
  // since OpenGL pixmaps have the bottom scanline first, and 
  // oiio expects the top scanline first in the image file.
  int scanlinesize = SourceImWidth * SourceImChannels * sizeof(unsigned char);
  if(!infile->read_image(TypeDesc::UINT8, &tmp_pixels[0] + (SourceImHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not read image from " << infilename << ", error = " << geterror() << endl;
    ImageInput::destroy(infile);
    return 0;
  }
 
 // get rid of the old OpenGL source and make a new one of the new size
  destroysource();
  
 // allocate space for the dest (contiguous approach)
  source = new Pixel*[SourceImHeight];
  if(source != NULL)
  source[0] = new Pixel[SourceImWidth * SourceImHeight];
  for(int i = 1; i < SourceImHeight; i++)
  source[i] = source[i - 1] + SourceImWidth;
 
 //  assign the read pixels to the dest
 int index;
  for(int row = 0; row < SourceImHeight; ++row) {
    for(int col = 0; col < SourceImWidth; ++col) {
      index = (row*SourceImWidth+col)*SourceImChannels;
      if (SourceImChannels==1){ 
        source[row][col].r = tmp_pixels[index];
        source[row][col].g = tmp_pixels[index];
        source[row][col].b = tmp_pixels[index];
        source[row][col].a = 255;
      }
      else{
        source[row][col].r = tmp_pixels[index];
        source[row][col].g = tmp_pixels[index+1];
        source[row][col].b = tmp_pixels[index+2];     
        if (SourceImChannels <4) // no alpha value is present so set it to 255
          source[row][col].a = 255; 
        else // read the alpha value
          source[row][col].a = tmp_pixels[index+3];     
      }
    }
  }
 
  // close the image file after reading, and free up space for the oiio file handler
  infile->close();
  ImageInput::destroy(infile);
  
  // set the pixel format to GL_RGBA and fix the # channels to 4  
  pixformat = GL_RGBA;  
  SourceImChannels = 4;

  // return image size in pixels
  return SourceImWidth * SourceImHeight;
}

//
// Routine to write the current framebuffer to an image file
//
void writeimage(string outfilename){
  // make a dest that is the size of the window and grab OpenGL framebuffer into it
   unsigned char local_pixmap[WinWidth * WinHeight * ImChannels];
   glReadPixels(0, 0, WinWidth, WinHeight, pixformat, GL_UNSIGNED_BYTE, local_pixmap);
  
  // create the oiio file handler for the image
  ImageOutput *outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // Open a file for writing the image. The file header will indicate an image of
  // width WinWidth, height WinHeight, and ImChannels channels per pixel.
  // All channels will be of type unsigned char
  ImageSpec spec(WinWidth, WinHeight, ImChannels, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }
  
  // Write the image to the file. All channel values in the dest are taken to be
  // unsigned chars. While writing, flip the image upside down by using negative y stride, 
  // since OpenGL pixmaps have the bottom scanline first, and oiio writes the top scanline first in the image file.
  int scanlinesize = WinWidth * ImChannels * sizeof(unsigned char);
  if(!outfile->write_image(TypeDesc::UINT8, local_pixmap + (WinHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }
  
  // close the image file after the image is written and free up space for the
  // ooio file handler
  outfile->close();
  ImageOutput::destroy(outfile);
}


//Write the image from the commandline argument
void writefromcmdline(string outfilename) {

  //Vertically flip
  for (int col = 0; col < DestImWidth; col++) {
        for (int row = 0; row < DestImHeight/2; row++) {
          swap(out[row][col], out[DestImHeight-1-row][col]);
      }
    }

  // create the oiio file handler for the image
  ImageOutput *outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // Open a file for writing the image. The file header will indicate an image of
  // width WinWidth, height WinHeight, and ImChannels channels per pixel.
  // All channels will be of type unsigned char
  ImageSpec spec(WinWidth, WinHeight, ImChannels, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }
  
  // Write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars. While writing, flip the image upside down by using negative y stride, 
  // since OpenGL pixmaps have the bottom scanline first, and oiio writes the top scanline first in the image file.
  if(!outfile->write_image(TypeDesc::UINT8, out[0])){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }

  // close the image file after the image is written and free up space for the
  // ooio file handler
  outfile->close();
  ImageOutput::destroy(outfile);

}

//
// Routine to display a dest in the current window
//
void displayimage(){
  // if the window is smaller than the image, scale it down, otherwise do not scale
  if(WinWidth < DestImWidth  || WinHeight < DestImHeight)
    glPixelZoom(float(VpWidth) / DestImWidth, float(VpHeight) / DestImHeight);
  else
    glPixelZoom(1.0, 1.0);
  
  // display starting at the lower lefthand corner of the viewport
  glRasterPos2i(0, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(DestImWidth, DestImHeight, pixformat, GL_UNSIGNED_BYTE, display[0]);
}

//
//   Display Callback Routine: clear the screen and draw the current image
//
void handleDisplay(){
  
  // specify window clear (background) color to be opaque black
  glClearColor(0, 0, 0, 1);
  // clear window to background color
  glClear(GL_COLOR_BUFFER_BIT);  
  
  // only draw the image if it is of a valid size
  if(DestImWidth > 0 && DestImHeight > 0)
    displayimage();
  
  // flush the OpenGL pipeline to the viewport
  glFlush();
}

//
//  Keyboard Callback Routine: 
//  'w' or 'W' to write the image to file
//  'q' or ESC - quit
//
void handleKey(unsigned char key, int x, int y){
  string outfilename;
  
  switch(key){
    case 'w':   // 'w' - write the image to a file
    case 'W':
      cout << "Output image filename? ";  // prompt user for output filename
      cin >> outfilename;
      writeimage(outfilename);
      break;

    case 'q':   // q or ESC - quit
    case 'Q':
    case 27:
      destroy();
      destroysource();
      exit(0);
      
    default:    // not a valid key -- just ignore it
      return;
  }
}

//
//  Reshape Callback Routine: If the window is too small to fit the image,
//  make a viewport of the maximum size that maintains the image proportions.
//  Otherwise, size the viewport to match the image size. In either case, the
//  viewport is centered in the window.
//
void handleReshape(int w, int h){
  float imageaspect = (float)DestImWidth / (float)DestImHeight; // aspect ratio of image
  float newaspect = (float)w / (float)h; // new aspect ratio of window
  
  // record the new window size in global variables for easy access
  WinWidth = w;
  WinHeight = h;
  
  // if the image fits in the window, viewport is the same size as the image
  if(w >= DestImWidth && h >= DestImHeight){
    Xoffset = (w - DestImWidth) / 2;
    Yoffset = (h - DestImHeight) / 2;
    VpWidth = DestImWidth;
    VpHeight = DestImHeight;
  }
  // if the window is wider than the image, use the full window height
  // and size the width to match the image aspect ratio
  else if(newaspect > imageaspect){
    VpHeight = h;
    VpWidth = int(imageaspect * VpHeight);
    Xoffset = int((w - VpWidth) / 2);
    Yoffset = 0;
  }
  // if the window is narrower than the image, use the full window width
  // and size the height to match the image aspect ratio
  else{
    VpWidth = w;
    VpHeight = int(VpWidth / imageaspect);
    Yoffset = int((h - VpHeight) / 2);
    Xoffset = 0;
  }
  
  // center the viewport in the window
  glViewport(Xoffset, Yoffset, VpWidth, VpHeight);
  
  // viewport coordinates are simply pixel coordinates
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, VpWidth, 0, VpHeight);
  glMatrixMode(GL_MODELVIEW);
}

void calculate() {
  double rgbToLms[3][3];
  rgbToLms[0][0] = 0.3811;
  rgbToLms[0][1] = 0.5783;
  rgbToLms[0][2] = 0.0402;
  rgbToLms[1][0] = 0.1967;
  rgbToLms[1][1] = 0.7244;
  rgbToLms[1][2] = 0.0782;
  rgbToLms[2][0] = 0.0241;
  rgbToLms[2][1] = 0.1288;
  rgbToLms[2][2] = 0.8444;

  Matrix3D rgbToLmsMatrix(rgbToLms);

  double lmsToLab1[3][3];
  lmsToLab1[0][0] = lmsToLab1[0][1] = lmsToLab1[0][2] = 1;
  lmsToLab1[1][0] = lmsToLab1[1][1] = 1;
  lmsToLab1[1][2] = -2;
  lmsToLab1[2][0] = 1;
  lmsToLab1[2][1] = -1;
  lmsToLab1[2][2] = 0;

  double lmsToLab2[3][3];
  lmsToLab2[0][0] = 1 / sqrt(3);
  lmsToLab2[0][1] = lmsToLab2[0][2] = lmsToLab2[1][0] = 0;
  lmsToLab2[1][1] = 1 / sqrt(6);
  lmsToLab2[1][2] = lmsToLab2[2][0] = lmsToLab2[2][1] = 0;
  lmsToLab2[2][2] = 1 / sqrt(2);

  Matrix3D lmsToLab1Matrix(lmsToLab1);
  Matrix3D lmsToLab2Matrix(lmsToLab2);

  Vector3D *sourceLabArray = new Vector3D[DestImHeight * DestImWidth];
  Vector3D *destLabArray = new Vector3D[DestImHeight * DestImWidth];

  //Convert RGB to LMS
  int index = 0;
  for(int row = 0; row < DestImHeight; row++) {
      for(int col = 0; col < DestImWidth; col++) {
        Vector3D sourceRGB(max(double(scaledsource[row][col].r)/255, 1.0/255), 
          max(double(scaledsource[row][col].g)/255, 1.0/255), 
          max(double(scaledsource[row][col].b)/255, 1.0/255));
        Vector3D sourceLMS = rgbToLmsMatrix * sourceRGB;
        //convert to log scale
        sourceLMS.x = log10(sourceLMS.x);
        sourceLMS.y = log10(sourceLMS.y);
        sourceLMS.z = log10(sourceLMS.z);

        Vector3D destRGB(max(double(dest[row][col].r)/255, 1.0/255), 
          max(double(dest[row][col].g)/255, 1.0/255), 
          max(double(dest[row][col].b)/255, 1.0/255));
        Vector3D destLMS = rgbToLmsMatrix * destRGB;

        //convert to log scale
        destLMS.x = log10(destLMS.x);
        destLMS.y = log10(destLMS.y);
        destLMS.z = log10(destLMS.z);

        //Convert from LMS to lαβ 
        Matrix3D sourceLmsToLabM = lmsToLab2Matrix * lmsToLab1Matrix;
        Vector3D sourceLab = sourceLmsToLabM * sourceLMS;

        Matrix3D destLmsToLabM = lmsToLab2Matrix * lmsToLab1Matrix;
        Vector3D destLab = destLmsToLabM * destLMS;

        sourceLabArray[index] = sourceLab;
        destLabArray[index] = destLab;
        index++;
      }
  }

  //Calculate mean values for l, α, and β source and destination images
  double sourceSumL = 0, sourceSumA = 0, sourceSumB = 0;
  double destSumL = 0, destSumA = 0, destSumB = 0;

  for(int i = 0; i < DestImHeight * DestImWidth; i++) {
    sourceSumL += sourceLabArray[i].x;
    sourceSumA += sourceLabArray[i].y;
    sourceSumB += sourceLabArray[i].z;

    destSumL += destLabArray[i].x;
    destSumA += destLabArray[i].y;
    destSumB += destLabArray[i].z;
  }

  double total = double(DestImHeight) * double(DestImWidth);

  double sourceMeanL = sourceSumL / total;
  double sourceMeanA = sourceSumA / total;
  double sourceMeanB = sourceSumB / total;

  double destMeanL = destSumL / total;
  double destMeanA = destSumA / total;
  double destMeanB = destSumB / total;

  //Calculate standard deviation for l, α, and β source and destination images
  double sourceVarianceL = 0, sourceVarianceA = 0, sourceVarianceB = 0;
  double destVarianceL = 0, destVarianceA = 0, destVarianceB = 0;

  for(int i = 0; i < DestImHeight * DestImWidth; i++) {
    sourceVarianceL += pow(sourceLabArray[i].x - sourceMeanL, 2);
    sourceVarianceA += pow(sourceLabArray[i].y - sourceMeanA, 2);
    sourceVarianceB += pow(sourceLabArray[i].z - sourceMeanB, 2);

    destVarianceL += pow(destLabArray[i].x - destMeanL, 2);
    destVarianceA += pow(destLabArray[i].y - destMeanA, 2);
    destVarianceB += pow(destLabArray[i].z - destMeanB, 2);
  }

  double sourceStdL = sqrt(sourceVarianceL / total);
  double sourceStdA = sqrt(sourceVarianceA / total);
  double sourceStdB = sqrt(sourceVarianceB / total);

  double destStdL = sqrt(destVarianceL / total);
  double destStdA = sqrt(destVarianceA / total);
  double destStdB = sqrt(destVarianceB / total);

  //Calculate ratio standard deviations
  double ratioStdL = sourceStdL / destStdL;
  double ratioStdA = sourceStdA / destStdA;
  double ratioStdB = sourceStdB / destStdB;

  //Calculate new data for destination lαβ
  for(int i = 0; i < DestImHeight * DestImWidth; i++) {
    destLabArray[i].x -= destMeanL;
    destLabArray[i].x *= ratioStdL;
    destLabArray[i].x += sourceMeanL;

    destLabArray[i].y -= destMeanA;
    destLabArray[i].y *= ratioStdA;
    destLabArray[i].y += sourceMeanA;

    destLabArray[i].z -= destMeanB;
    destLabArray[i].z *= ratioStdB;
    destLabArray[i].z += sourceMeanB;
  }

  //Convert lαβ to LMS
  double labToLms1[3][3];
  labToLms1[0][0] = sqrt(3) / 3;
  labToLms1[0][1] = labToLms1[0][2] = labToLms1[1][0] = 0;
  labToLms1[1][1] = sqrt(6) / 6;
  labToLms1[1][2] = labToLms1[2][0] = labToLms1[2][1] = 0;
  labToLms1[2][2] = sqrt(2) / 2;

  double labToLms2[3][3];
  labToLms2[0][0] = labToLms2[0][1] = labToLms2[0][2] = 1;
  labToLms2[1][0] = labToLms2[1][1] = 1;
  labToLms2[1][2] = -1;
  labToLms2[2][0] = 1;
  labToLms2[2][1] = -2;
  labToLms2[2][2] = 0;

  Matrix3D labToLms1Matrix(labToLms1);
  Matrix3D labToLms2Matrix(labToLms2);

  //Convert from LMS to RGB
  double lmsToRgb[3][3];
  lmsToRgb[0][0] = 4.4679;
  lmsToRgb[0][1] = -3.5873;
  lmsToRgb[0][2] = 0.1193;
  lmsToRgb[1][0] = -1.2186;
  lmsToRgb[1][1] = 2.3809;
  lmsToRgb[1][2] = -0.1624;
  lmsToRgb[2][0] = 0.0497;
  lmsToRgb[2][1] = -0.2439;
  lmsToRgb[2][2] = 1.2045;

  Matrix3D lmsToRgbMatrix(lmsToRgb);

  Vector3D *destRGBArray = new Vector3D[DestImHeight * DestImWidth];

  for(int i = 0; i < DestImHeight * DestImWidth; i++) {
    Matrix3D labToLmsM = labToLms2Matrix * labToLms1Matrix;
    destLabArray[i] = labToLmsM * destLabArray[i];

    //Convert back to linear space
    destLabArray[i].x = pow(10, destLabArray[i].x);
    destLabArray[i].y = pow(10, destLabArray[i].y);
    destLabArray[i].z = pow(10, destLabArray[i].z);

    //Convert LMS back to RGB
    destRGBArray[i] = lmsToRgbMatrix * destLabArray[i];
  }

  display = new Pixel*[DestImHeight];
  if(display != NULL)
   display[0] = new Pixel[DestImWidth * DestImHeight];
  for(int i = 1; i < DestImHeight; i++)
   display[i] = display[i - 1] + DestImWidth;

  index = 0;
  for(int row = 0; row < DestImHeight; row++) {
      for(int col = 0; col < DestImWidth; col++) {
        display[row][col].r = min(abs(destRGBArray[index].x * 255), float(255));
        display[row][col].g = min(abs(destRGBArray[index].y * 255), float(255));
        display[row][col].b = min(abs(destRGBArray[index].z * 255), float(255));

        index++;
      }
  }

}

/*
Multiply M by a scale matrix of scaleX and scaleY
*/
void Scale(Matrix3D &M, double scaleX, double scaleY) {
  Matrix3D S; 
  S[0][0] = scaleX;
  S[1][1] = scaleY;

  M = S * M;
}

/*
   Main program to read an image file, then ask the user
   for transform information, transform the image and display
   it using the appropriate warp.  Optionally save the transformed
   images in  files.
*/
int main(int argc, char *argv[]){

  // set up the default window and empty dest if no image or image fails to load
  WinWidth = DEFAULTWIDTH;
  WinHeight = DEFAULTHEIGHT;
  DestImWidth = 0;
  DestImHeight = 0;
  SourceImWidth = 0;
  SourceImHeight = 0;

  // initialize transformation matrix to identity
  Matrix3D M;

  if(argc == 3 || argc == 4) {
    //Read in source image
    readsourceimage(argv[1]);
    //Read in other image
    readimage(argv[2]);

    if(SourceImWidth != DestImWidth || SourceImHeight != DestImHeight) {
      //Scale source image to same size as destination
      Scale(M, double(DestImWidth)/double(SourceImWidth), double(DestImHeight)/double(SourceImHeight));

      // code to perform inverse mapping (4 steps)
      // set each corner of the image
      Vector3D corner1(0, 0, 1);
      Vector3D corner2(SourceImWidth, 0, 1);
      Vector3D corner3(0, SourceImHeight, 1);
      Vector3D corner4(SourceImWidth, SourceImHeight, 1);

      Vector3D v0 = M * corner1;
      v0.x /= v0.z;
      v0.y /= v0.z;
     
      Vector3D v1 = M * corner2;
      v1.x /= v1.z;
      v1.y /= v1.z;

      Vector3D v2 = M * corner3;
      v2.x /= v2.z;
      v2.y /= v2.z;

      Vector3D v3 = M * corner4;
      v3.x /= v3.z;
      v3.y /= v3.z;

      //Find the top, bottom, left, and right points
      double top = v0.y;
      if(v1.y > top) {
        top = v1.y;
      }
      if(v2.y > top) {
        top = v2.y;
      }
      if(v3.y > top) {
        top = v3.y;
      }
      double bottom = v0.y;
      if(v1.y < bottom) {
        bottom = v1.y;
      }
      if(v2.y < bottom) {
        bottom = v2.y;
      }
      if(v3.y < bottom) {
        bottom = v3.y;
      }
      double left = v0.x;
      if(v1.x < left) {
        left = v1.x;
      }
      if(v2.x < left) {
        left = v2.x;
      }
      if(v3.x < left) {
        left = v3.x;
      }
      double right = v0.x;
      if(v1.x > right) {
        right = v1.x;
      }
      if(v2.x > right) {
        right = v2.x;
      }
      if(v3.x > right) {
        right = v3.x;
      }

      //Build transform matrix
      double coefs[3][3];
      coefs[0][0] = coefs[1][1] = coefs[2][2] = 1.0;
      coefs[2][0] = coefs[2][1] = coefs[0][1] = coefs[1][0] = 0.0;
      coefs[1][2] = -bottom;
      coefs[0][2] = -left;

      Matrix3D tr(coefs);
      
      //Find new height and width
      W2 = right - left;
      H2 = top - bottom;

      //Allocate memory for scaled image
      scaledsource = new Pixel*[H2];
      scaledsource[0] = new Pixel[H2*W2];
      for(int i = 1; i < H2; i++)
        scaledsource[i] = scaledsource[i - 1] + W2;

      M = tr * M;
      Matrix3D invM = M.inverse();

      for(int y = 0; y < H2; y++) {
        for(int x = 0; x < W2; x++) {
          //map the pixel coordinates
          Vector3D pixel_out(x, y, 1);
          Vector3D pixel_in = invM * pixel_out;

          //normalize the dest
          int u = pixel_in.x / pixel_in.z;
          int v = pixel_in.y / pixel_in.z;
          if(u >= 0 && u < SourceImWidth && v >= 0 && v < SourceImHeight) {
            scaledsource[y][x] = source[v][u];
          }

        }
      }
    } else {
      
      scaledsource = new Pixel*[DestImHeight];
      scaledsource[0] = new Pixel[DestImHeight*DestImWidth];
      for(int i = 1; i < DestImHeight; i++)
        scaledsource[i] = scaledsource[i - 1] + DestImWidth;

      for(int x = 0; x < DestImHeight; x++) {
        for(int y = 0; y < DestImWidth; y++) {
          scaledsource[x][y] = source[x][y];
        }
      }

    }

    //Perform calculations
    calculate();

    WinWidth = DestImWidth;
    WinHeight = DestImHeight;

    //Write the image to inputted file
    if(argc == 4) {
        //Allocate space for output pixmap
      out = new Pixel*[DestImHeight];
      if(out != NULL)
      out[0] = new Pixel[DestImWidth * DestImHeight];
      for(int i = 1; i < DestImHeight; i++)
      out[i] = out[i - 1] + DestImWidth;

      //Copy display to out pixmap
      for (int row = 0; row < DestImHeight; row++) {
        for (int col = 0; col < DestImWidth; col++) {
          out[row][col] = display[row][col];
        }
      }

      writefromcmdline(argv[3]);
    }


    //your code to display the warped image
    // start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(WinWidth, WinHeight);
    glutCreateWindow("Color Transfer");
    
    // set up the callback routines to be called when glutMainLoop() detects
    // an event
    glutDisplayFunc(handleDisplay); // display callback
    glutKeyboardFunc(handleKey);    // keyboard key press callback
    glutReshapeFunc(handleReshape); // window resize callback

    // Enter GLUT's event loop
    glutMainLoop();
  }

  return 0;
}
