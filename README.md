# Color Transfer Between Images
+ This code reads in a source image and a destination image, then using the approach in Reinhard et al, transfers the colors from the source image to the destination image, then displays the result.
+ Recommended to use images that are similar otherwise there may be artifacts.

### How to run:
1. Compile with 'make'
2. Type ./colortransfer source.png destination.png [outimage.png]

### Once image is displayed:
+ 'W' or 'w' to write the displayed image to a file
+ 'q' or 'esc' is to quit

### Explanation:
In this project, I used the approach in [Reinhard et al](https://5d73342f-a-1e6e9713-s-sites.googlegroups.com/a/g.clemson.edu/cpsc-6040/schedule/ColorTransfer.pdf?attachauth=ANoY7cppcLFf8RioKkBDDN5K5Q6pxGKoI9sZ83231IVzfunJ2323iLUsEaWKDUzsbDl8vYc5C4jy7H1PWyCnnFjAEiZJY9HQdsGtNa3ZLqpoT_jGx1AtfjDnyaWiP4CI4nmbZ4vwIN1TOKDbSLjEW9e0AN0a2hp0pHdR48kV-UeymIOQWz5Kqiiq0aqkCE5PRoDYY3-3m_Z0E5hXRjZwSZl_elku5cAb0Z1o5CZE_Xyi8yG19v1_XVw%3D&attredirects=1) to transfer colors from a source image to a destination image.

### Results:
Source                     |  Destination              |  Final
:-------------------------:|:-------------------------:|:-------------------------:
![](https://github.com/Drakyoid/color-transfer/blob/master/images/sky.png?raw=true)   | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/sunset.png?raw=true) | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/sunset_sky.png?raw=true)
![](https://github.com/Drakyoid/color-transfer/blob/master/images/deathvalley2.jpg?raw=true)   | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/deathvalley.jpg?raw=true) | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/deathvalley_squared.png?raw=true)
![](https://github.com/Drakyoid/color-transfer/blob/master/images/desert.jpg?raw=true)   | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/mountains.jpg?raw=true) | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/desert_mountains.png?raw=true)
![](https://github.com/Drakyoid/color-transfer/blob/master/images/beach.jpg?raw=true)   | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/starrynight.jpg?raw=true) | ![](https://github.com/Drakyoid/color-transfer/blob/master/images/vibrant_starrynight.png?raw=true)
