# azure-storage-gst-plugin

This is a gstreamer sink for azure storage.

## Prerequisites

You'll need `azure-storage-cpplite` and `gstreamer`.

* Follow [azure-storage-cpplite](https://github.com/Azure/azure-storage-cpplite)'s readme and install it locally.
* [Install gstreamer](https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c#) on your platform.

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

(gst-inspect-1.0:22844): GStreamer-CRITICAL **: 05:48:42.070: Padname sink is not unique in element (null), not adding
Factory Details:
  Rank                     none (0)
  Long-name                Azure storage sink
  Klass                    Generic
  Description              Write stream into azure blob storage.
  Author                   Eugene Chen-yijunc@microsoft.com
#... and many other details
```

## Using the plugin

TODO :)