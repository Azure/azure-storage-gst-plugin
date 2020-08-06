# azure-storage-gst-plugin

This repo contains gstreamer plugins for azure storage.

## Prerequisites

* [Install gstreamer](https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c#) on your platform.
* This repo also depends on [azure-storage-cpplite](https://github.com/Azure/azure-storage-cpplite).
Code for this repo will be automatically built by cmake so that you won't have to install it yourself, but you'll still need to install its
**dependencies** on your system. Check out [here](https://github.com/Azure/azure-storage-cpplite#install-the-dependencies-eg-on-ubuntu).

This repo is tested under Ubuntu 18.04 & 20.04.

## Build

This project use cmake to build.

```bash
mkdir build
cd build
cmake --build .
# to install the plugin system-wide
sudo cmake --build . --target install
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
  azuresrc: Azure storage source

  2 features:
  +-- 2 elements
```

## Using the plugin

To test it with random snow output:

```bash
cd build  # goto the library's location
# the following three parameters are mandatory
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e videotestsrc pattern=snow ! x264enc ! matroskamux ! \
  azuresink account-name="your account name" account-key="your account key" \
    location="container_name/blob_name"
```

Or if you have a webcam...

```bash
cd build
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e v4l2src ! videoconvert ! x264enc ! flvmux ! \
  azuresink account-name="your account name" account-key="your account key" \
    location="container_name/blob_name"
```