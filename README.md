# Privacy Protection Algorithm (PPA) 
This repository contains code and instructions for building two tools for
processing vehicle trip, or trajectory, data. This tool was developed to process large databases of trips generated
during connected vehicle pilot studies, but the **Privacy Protection Algorithm can be used on any type of moving object
data defined as time-sequences of geolocations that also includes heading and speed.** Heading and speed can be derived
after the initial trip record, but currently the algorithm requires time, latitude, longitude, speed, and heading as a
minimum for processing.

The [Privacy Protection Algorithm Manual](./docs/cvdi-user-manual.md) describes the GUI tool's interface, and the
parameter settings and their influence on algorithm operation. Users should review that material prior to using either
tool.

# Table of Contents
- [Getting Involved](#getting-involved)
- [Release Notes](#release-notes)
- [Dependancies and Prerequisites](#dependancies-and-prerequisitives)
- [Building the Privacy Protection Application](#building-the-privacy-protection-application)
- [Post Build Application Installation Instructions](#post-build-application-installation)
- [Running the Command Line Tool](#running-the-command-line-tool)
- [Issues, Comments, and Additional Inquiries](#issues-and-questions)

# Getting Involved

This project, sponsored by the U.S. Department of Transportation, limits the risk of traveler
identification when using valuable mobility data that will aid the research community. To project [factsheet](https://www.its.dot.gov/factsheets/pdf/Managing_ITS_DataPrivacyRisk.pdf) contains more information. You can become involved in this project in several
ways:

- **Pull the code and try it out!**  The [instructions](#introduction) that immediately follow will get you started.  [Build](#building-the-privacy-protection-application) instructions are also provided.  Let us know if you need help.
    - Github has [instructions](https://help.github.com/articles/signing-up-for-a-new-github-account) for setting up an account and getting started with repositories.
- If you would like to improve this code base or the documentation, [fork the project](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application#fork-destination-box) and submit a pull request (see the next section).
- If you find a problem with the code or the documentation, please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues/new).
- If the PPA does not solve your mobility data privacy problem, please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues/new) and prefix the issue title with **[New Feature]**. We would like to help.

## Introduction

The PPA project uses the [Pull Request Model](https://help.github.com/articles/using-pull-requests). This involves the following project components:

- The Organization Privacy Protection Application Project's [master branch](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application).
- PPA releases are made via [tags](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/releases) out of master.
- A personal GitHub account.
- A fork of a project release tag or master branch in your personal GitHub account.

A high level overview of our model and these components is as follows. All work will be submitted via pull requests.
Developers will work on branches on their personal machines (local clients), push these branches to their **personal GitHub repos** and issue a pull
request to the organization PPA project. One the project's main developers must review the Pull Request and merge it
or, if there are issues, discuss them with the submitter. This will ensure that the developers have a better
understanding of the code base *and* we catch problems before they enter `master`. The following process should be followed:

## Initial Setup

1. If you do not have one yet, create a personal (or organization) account on GitHub (assume your account name is `<your-github-account-name>`).
1. Log into your personal (or organization) account.
1. Fork [private-protection-application](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/fork) into your personal GitHub account.
1. On your computer (local client), clone the master branch from you GitHub account:
```bash
$ git clone https://github.com/<your-github-account-name>/privacy-protection-appliation.git
```

### Additional Resources for Initial Setup
  
- [About Git Version Control](http://git-scm.com/book/en/v2/Getting-Started-About-Version-Control)
- [First-Time Git Setup](http://git-scm.com/book/en/Getting-Started-First-Time-Git-Setup)
- [Article on Forking](https://help.github.com/articles/fork-a-repo)

## Development in Your Fork

As an example, assume you are enhancing the existing code or documentation, or working on an
[issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues).  Complete the following
steps on your computer (local client):

1. Add an upstream remote repository pointing to the Privacy Protection Application project repository.
    ```bash
    $ git remote add upstream https://github.com/usdot-its-jpo-data-portal/privacy-protection-application.git
    ```
1. Pull from the upstream remote repository to get the latest master branch.
    ```bash
    $ git pull --rebase upstream master
    ```
1. Create a branch on your client (your forked repository).
    ```bash
    $ git branch enhancement
    ```
   where ```enhancement``` is the name you want to use for the branch.
1. Switch to this branch
    ```bash
    $ git checkout enhancement
    ```
1. Make the changes need to enhance the code. Note that before doing a pull request (below) you also want to ensure that any changes that have been accepted upstream can be integrated with any changes you are making now, so perform the following command in your forked repository branch prior to issuing a pull request:
    ```bash
    $ git pull upstream master
    ```
1. When you have completed coding and integrating the upstream code, run the unit tests on your client in your build directory.
    ```bash
    $ cd ${BUILD_DIR}/cv-lib-test
    $ ./cvlib_tests
    ```
1. When all the tests pass you can **add, commit, and push your updates to your `enchancement` branch in your fork.** Follow these steps:
    1. Check which files have been changed
        ```bash
        $ git status
        ```
    1. Add the files that changed based on your enhancement, or that you want to include in the pull request.
        ```bash
        $ git add <file1> <file2>
        ```
    1. Commit the files with a message describing the enhancement or bug fix.
        ```bash
        $ git commit -m "improved this aspect of the code."
        ```
    1. Push the branch changes (enhancements) to your GitHub account:
        ```bash
        $ git push origin enhancement
        ```

## Create A Pull Request

1. After you have pushed your `enhancement` branch to your GitHub account, issue a pull request on GitHub. Details on how to perform a [pull request](https://help.github.com/articles/using-pull-requests) are on GitHub.
1. If the pull request is closing an issue, include a comment with `fixes #issue_number`.  This comment will close the issue when the pull request is merged.  For more information see [here](https://github.com/blog/1506-closing-issues-via-pull-requests).
1. One of the main project developers will review your pull request and either merge it, or send you feedback. **Do not merge your own pull request.** Code review is essential. If you have not received feedback on your pull request in a timely fashion, contact us via email.
1. Once your pull request has been reviewed and merged (possibly closing an issue), your enhancement will now be part of the privacy protection application project `master` branch.
1. On your client machine, you can delete your branch.
    ```bash
    $ git branch -d enhancement
    ```
1. Pull from the privacy protection application project's `master` branch to have your changes reflected in your local (laptop/desktop) `master` branch:
    ```bash
    $ git checkout master
    $ git pull --rebase upstream master
    ```
1. To include these in your fork's master branch, push them to your GitHub account.
    ```git push origin master```
1. At this point the three master branches (one on organization, one on your GitHub account, and one on your client) are all in sync.

# Release Notes

## Initial Release
- Release of the GUI and the CLI for public use and evaluation.

# Dependancies and Prerequisites

- Install [Git](https://git-scm.com/) or you will not be able to interact with this repository.
- Install [CMake](https://cmake.org) to build these applications.
- [Catch](https://github.com/philsquared/Catch) is used for unit testing, but it is included in the repository.
- [Node.js](https://nodejs.org/en/download) is needed to build and run the privacy protection application; this should
  also install the required Node Package Manager (`npm`). See below for more information. On OS X, Node.js can also be installed
  using MacPorts and Brew.
    - (Ubuntu Install of Node.js)
        ```bash
        $ curl -sL https://deb.nodesource.com/setup_9.x | sudo -E bash -
        $ sudo apt install -y nodejs
        ```
    - The final command will install `node_version.h` in `/usr/include/node/`
- If building on Windows, [Visual C++ Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools) are needed. You may run into [this issue](https://stackoverflow.com/questions/33743493/why-visual-studio-2015-cant-run-exe-file-ucrtbased-dll). We found that having Win10 SDK installed fixed the issue on our system.
- We built the Windows application using cmake generated Nmake files. Other build environments were not tested.

# Building the Privacy Protection Application

The following procedure outlines a fresh install after the above dependencies have been installed.  It builds both the
command line tool and the graphical user interface form of the application. By following the procedures the repository
will reside in the `$PPA_BASE_DIR`. An out of source build directory, `$PPA_BUILD_DIR`), will also be created.

The command line tool executable will be located in `$PPA_BUILD_DIR`.

- CLI will not build unless it has nan.h
- node installation is not enough
- good direction on node installation on linux: https://nodejs.org/en/download/package-manager/
- installed nan and it didn't work.

1. Install Node.js if not installed (see [Dependencies](#dependancies-and-prerequisites)). You can check for installation using these version commands:
    ```bash
    $ node -v
    $ npm -v
    ```
1. Make a directory (`$PPA_BASE_DIR`) where the repository will be cloned and clone the repository. 
    ```bash    
    $ git clone https://github.com/usdot-its-jpo-data/privacy-protection-application ${PPA_BASE_DIR}
    ```
1. Make a directory (`$PPA_BUILD_DIR`) to build the application (and CLI tool). Out of source builds are recommended.
    ```bash 
    $ mkdir ${PPA_BUILD_DIR}
    ```
1. Using Node Package Manager (`npm`), install the following build tools and dependencies:
    - Install the CMake-like build system for Node.js called [Cmake.js](https://github.com/cmake-js/cmake-js)
        ```bash
        $ sudo npm install --save -g cmake-js
        ```
    - Install the [Electron Package Manager](https://github.com/electron-userland/electron-packager)
        ```bash
        $ sudo npm install  --save-dev -g electron-packager
        ```
    - Install [Electron](https://github.com/electron/electron). Running the install command in the `PPA_BASE_DIR` avoids an error on linux.
        ```bash
        $ cd ${PPA_BASE_DIR}
        $ npm install  --save-dev --save-exact electron
        ```
    - Using [Native Abstractions for Node.js](https://www.npmjs.com/package/nan). Running the install command in the `PPA_BASE_DIR` avoids an error on linux.
        ```bash
        $ cd ${PPA_BASE_DIR}
        $ npm install --save nan
        ```
1. The file `package.json` must be in the `$PPA_BASE_DIR` to build using `cmake-js`. *It should be included when you clone the repository.*
1. Make a directory (`$PPA_APP_DIR`) to install the application.
1. Build the tools. This will build both the GUI and CLI tools.
    ```bash
    $ cmake-js -d ${PPA_BASE_DIR} -O ${PPA_BUILD_DIR} -a x64 -r electron -v 1.4.15
    // the command line tool is built at this point.
    $ electron-packager --icon=${PPA_BUILD_DIR}/cv-gui-electron/${PPA_BUILD_DIR}/Release/electron-app/images/${OS_IMAGE} --overwrite --out ${PPA_APP_DIR} --electron-version=1.7.12 ${PPA_BUILD_DIR}/cv-gui-electron/${PPA_BUILD_DIR}/Release/electron-app
    ```
    Where (`$OS_IMAGE`) is the images type for your operating system. (Windows = *.ico, OSX = *.icns, Linux = *.png). Make sure to substitute your architecture (x64 above) and Electron version (1.4.15 above) in previous commands.

# Post Build Installation

Installation of the Route Sanitizer application involves downloading the
application zip file, unzipping the file, and creating a shortcut for
easy application launch. The process is almost identical on Windows
7/8/10 and Mac OS X.  Windows screen shots are given in the main body of
the user’s manual while Mac screen shots are given in Appendix A. The
numbers in red circles highlight areas in the figures that pertain to
certain instructions: the bracketed number in the instruction
corresponds to the circled number in the figure. The steps that follow
outline this process:

1. Move the entire RouteSanitizer folder to any desired location on
your computer before creating any shortcuts. The Download folder is
generally not a good location for the unzipped application. We suggest
creating a project folder and putting the RouteSanitizer application
folder inside the project folder. Separate folder can be created inside
the project folder for data files and/or result files.

1. Create Route Sanitizer shortcuts.

    1. Windows:

        1. In the “RouteSanitizer-win32-x64” folder find the file named “RouteSanitizer.exe” or “RouteSanitizer”. Note that the file name extension “.exe” might not be visible.

        1. Right click on the RouteSanitizer file, select “Send to” in the first pop-up menu and then “Desktop (create shortcut)” in the second pop-up menu. (For pictorial instructions, see: <http://www.thewindowsclub.com/create-desktop-shortcut-windows-10> .) If you want to rename the shortcut, right click anywhere on the desktop shortcut and select “Rename”. The name text can then be edited.

        1. If you would also like the RouteSanitizer shortcut to appear in the Windows task bar, just drag the RouteSanitizer desktop shortcut to the desired location on the Windows task bar.

    1. Mac:

        1. In the “RouteSanitizer- darwin -x64” folder find the file named “RouteSanitizer”. Note that on Macs the “.app” file name extension is not visible by default even if extensions are visible for other file types.

        1. Right click on the “RouteSanitizer” application and select “Make Alias” from the popup menu. Drag the shortcut “RouteSanitizer alias” created in the “RouteSanitizer- darwin -x64” folder to the desktop.  (For pictorial instructions, see: <http://www.macworld.co.uk/how-to/mac-software/how-create-shortcuts-on-mac-3613491/> .) If you want to rename the shortcut, right click on the name portion of the desktop shortcut and select “Rename”. The name text can then be edited.

        1. If you would also like the RouteSanitizer shortcut to appear in the Mac dock bar, just drag the RouteSanitizer desktop shortcut to the desired location on the dock bar.

1. Launch the RouteSanitizer application.

    1. To launch the RouteSanitizer application, just double click on
the desktop shortcut or single click on the taskbar (Windows) or dock
(Mac) shortcut. The application can also be started by double clicking
on the application executable in RouteSanitizer folder.

    1. When running a new version of the RouteSanitizer application for
the first time on a Mac, you might get a popup window that says
“RouteSanitizer can’t be opened because it is from an unidentified
developer”. In this case, right click on the application or shortcut and
select “Open” from the popup menu. This time the unidentified developer
popup window will have an “Open” button. Click the “Open” button to
start the application. The next time the application should start by
double clicking on the shortcut or application.

To un-install the RouteSanitizer application, just delete the entire
RouteSanitizer folder. No special un-install procedure is required on
either Windows or Mac computers.

To install a new version of RouteSanitizer, delete the old version and
then go through the same install procedure for the new version. If you
want to keep multiple versions of RouteSanitizer, rename the folder of
the older version before installing the new version. You may also need
to delete the shortcut to the old version and make a new one with a
different name if desired.

# Running the Command Line Tool

In addition to the GUI form of the tool, the Privacy Protection Tool can be used from the command line after it has been built. The usage information follows. **Command line options override parameters specified in the configuration file.** When using the producer above the CLI tool will be located in `${PPA_BUILD_DIR}/cl-tool`.

```bash
Usage: cv_di [OPTIONS] SOURCE
OPTIONS
 -c, --config         A configuration file for de-identification.
 -o, --out_dir        The output directory (default: working directory).
 -n, --count_pts      Print summary of the points after de-identification to standard error.
 -q, --quad           The file .quad file containing the circles defining the regions.
 -k, --kml_dir        The KML output directory (default: working directory).
 -t, --thread         The number of threads to use (default: 1 thread).
 -h, --help           Print this message.
```

SOURCE is a text file that contains the path to a trip file on each line. As an example, the following command uses a configuration file to process the trips contained in SOURCE:

```bash
$ ./cv_di -c <configuration file> <source-file>
```

# Running The Library Tests

The library tests are designed to cover most of the functions and routines used in the Privacy Protection Tool. To run the compiled library tests, you need to change directory into the test directory and execute the test command:

```bash
$ cd cv-lib-test
$ ./cvlib_tests
```

# Issues and Questions

If you need to contact the principal investigator or developers for this project, 
please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issue).  
Please consult the project [factsheet](https://www.its.dot.gov/factsheets/pdf/Managing_ITS_DataPrivacyRisk.pdf) for additional information and contacts.

