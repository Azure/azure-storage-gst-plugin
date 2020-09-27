# azure-storage-gst-plugin

This repo contains gstreamer plugins for azure storage.

## Prerequisites

* [Install gstreamer](https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c#) on your platform.
* This repo also depends on [azure-storage-cpplite](https://github.com/Azure/azure-storage-cpplite).
Code for this repo will be automatically built by cmake so that you won't have to install it yourself, but you'll still need to install its
**dependencies** on your system. Check out [here](https://github.com/Azure/azure-storage-cpplite#install-the-dependencies-eg-on-ubuntu).

This repo is tested under Ubuntu 18.04 & 20.04.

## Build

This project uses cmake to build.

```bash
mkdir build
cd build
# to build the plugin
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

Use `gst-inspect-1.0 azuresink` or `gst-inspect-1.0 azuresrc` to inspect the plugin.
If you install the plugin system-wide, you should be able to see the plugin without any additional parameters, or you can specify `GST_PLUGIN_PATH`:

```bash
cd build
GST_PLUGIN_PATH=. gst-inspect-1.0 azuresrc
GST_PLUGIN_PATH=. gst-inspect-1.0 azuresink
```

## Using the plugin

To write random snow output to an append blob:

```bash
cd build  # goto the library's location
# the following three parameters are mandatory
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e videotestsrc pattern=snow ! x264enc ! matroskamux ! \
  azuresink account-name="your account name" account-key="your account key" \
    location="container_name/blob_name" blob-type="append"
```

Or write your webcam input to a block blob:

```bash
cd build
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e v4l2src ! videoconvert ! x264enc ! flvmux ! \
  azuresink account-name="your account name" account-key="your account key" \
    location="container_name/blob_name" blob-type="block"
```

You can also watch video from azure storage:

```bash
cd build
GST_PLUGIN_PATH=. gst-launch-1.0 -v -e azuresrc location="container_name/blob_name" \
  account-name="your account name" account-key="your account key" ! decodebin ! autovideosink
```

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
