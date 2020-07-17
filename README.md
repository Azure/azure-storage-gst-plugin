# azure-storage-gst-plugin

This is a gstreamer sink for azure storage.

## Prerequisites

You'll need `azure-storage-cpplite` and `gstreamer`.

* Follow [azure-storage-cpplite](https://github.com/Azure/azure-storage-cpplite)'s readme and install it locally.
  * Note that you'll need to build **shared library**. Turn on `BUILD_SHARED_LIBS` when building.
* [Install gstreamer](https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c#) on your platform.

This repo is tested under Ubuntu 18.04 & 20.04.

## Build

This project use cmake to build.

```bash
mkdir build
cd build
cmake ..
make # and then run...
```

To inspect the built plugin:

```bash
# make sure that plugin shared library is built
$ ls lib*
libgstazureelements.so
$ gst-inspect-1.0 ./libgstazureelements.so
Plugin Details:
  Name                     azureelements
  Description              Azure Storage Elements
  Filename                 ./libgstazureelements.so
  Version                  1.0
  License                  LGPL
  Source module            Gstreamer Azure Storage Package
  Binary package           Gstreamer Azure Storage Package
  Origin URL               https://www.azure.com/

  azuresink: Azure storage sink

  1 features:
  +-- 1 elements
$ GST_PLUGIN_PATH=. gst-inspect-1.0 azuresink
Factory Details:
  Rank                     none (0)
  Long-name                Azure storage sink
  Klass                    Generic
  Description              Write stream into azure blob storage.
  Author                   Eugene Chen-yijunc@microsoft.com
#... and many other details
```

## Using the plugin

To test it with random snow output:

```bash
cd build  # goto the library's location
# the following four parameters are mandatory
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e videotestsrc pattern=snow ! x264enc ! matroskamux ! \
  azuresink account-name="your account name" account-key="your account key" \
    container-name="your container name" blob-name="output blob name"
```

Or if you have a webcam...

```bash
cd build
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e v4l2src ! videoconvert ! x264enc ! flvmux ! \
  azuresink account-name="your account name" account-key="your account key" \
    container-name="your container name" blob-name="output blob name"
```