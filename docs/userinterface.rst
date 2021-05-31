==============
User Interface
==============
.. image:: https://i.imgur.com/J2kAatK.png

Print Customization
---------------------------

.. figure:: https://i.imgur.com/N3GkRIh.png
    :align: right
    :figwidth: 300px

The print customization window determines high level print features.

**Select Resin:**
Currently not in use.

**Select Projection Mode:**
Select between Pattern On The Fly (POTF) and video pattern mode.

**Select Printer:**
Select between a standard CLIP printer and DI-CLIP printer.

**Print Type:**
Select between stepped or continuous motion.

**Bit Depth:**
Select the bit depth of your input images, the default is 1-bit
binary images. Increasing the bit depth allows for varying depths of
grayscale up to 8-bit grayscale images.

**Max Image Upload:**
Determines the max number of images to upload at one time when in POTF
mode. With 1-bit images this caps out at 400 images, with 8-bit images
this caps out at 50 images. User may want to keep the max image upload low
to avoid print discontinuities caused by long upload times.

General Print Settings
---------------------------

.. figure:: https://i.imgur.com/sptkfoX.png
    :align: right
    :figwidth: 300px

**Initial Exposure Time:**
The initial exposure time determines the exposure time for the first layer
of the print. This allows the print to properly adhere to the build platform.

**Starting Position:**
Simply determines the starting position of the print, this value should be at the
deadzone thickness from the window. Any movement relative to this value will be negative
each layer will move the stage closer to 0.

**Slice Thickness:**
Determines the layer thickness of the print. If set to 10 um each image file will result
in a 10um thick layer.

**Auto Mode Settings:**
Determines print settings automatically based on desired print speed and height. Not in much use
at the moment.

Light Engine Control
""""""""""""""""""""""
* Exposure time determines how long the light engine will expose for each layer. If a print script is active it will take over exposure time control.
* UV Intensity determines the intensity of the UV LEDs in the light engine (ranges from 1-255).
* Dark time determines the time in between exposures, dark time is used for stage movement and timing overhead.

Stage Control
"""""""""""""""""""""
* Stage velocity determines the velocity of the stage. Does not have an active effect on the print unless set below 1 mm/s.
* Stage acceleration determines the acceleration of the stage. Does not have an active effect on the print unless set below 3 mm/s^2.
* Max end of run determines the upper limit of stage movement, this value should be set to be the same height as the build window.
* Min end of run determines the lower limit of stage movement. Default is set to 0, changing this variable is not reccommended unless you printing object with heights greater than max end of run.

Pump Control
"""""""""""""""""""
Pump control is currently not in use.

Peripheral Controls
---------------------------
.. figure:: https://i.imgur.com/CBlX0mU.png
    :align: right
    :figwidth: 300px


Input Files
---------------------------
.. figure:: https://i.imgur.com/CBlX0mU.png
    :align: right
    :figwidth: 300px

Print Controls
---------------------------
.. figure:: https://i.imgur.com/Nz9QY79.png
    :align: right
    :figwidth: 300px

Print Log
-------------------
.. figure:: https://i.imgur.com/qYGeweg.png
    :align: right
    :figwidth: 300px

Print Monitoring
---------------------------
