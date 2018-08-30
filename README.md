TODO change references to master branch to whatever we call this ..

# Privacy Protection Algorithm (PPA) 
This repository contains code and instructions for building a tool for
processing vehicle trip, or trajectory, data. This tool was developed to process large databases of trips generated
during connected vehicle pilot studies, but the **Privacy Protection Algorithm can be used on any type of moving object
data defined as time-sequences of geolocations that also includes heading and speed.** Heading and speed can be derived
after the initial trip record, but currently the algorithm requires time, latitude, longitude, speed, and heading as a
minimum for processing.

# Table of Contents
- [Getting Involved](#getting-involved)
- [Release Notes](#release-notes)
- [Dependencies and Prerequisites](#dependancies-and-prerequisitives)
- [Building the OSM CSV Network File](#building-the-osm-csv-network-file)
- [Running the Privacy Protection Container](#running-the-privacy-protection-container)
- [Issues, Comments, and Additional Inquiries](#issues-and-questions)

# Getting Involved

This project is one effort, sponsored by the U.S. Department of Transportation, to limit the risk of traveler
identification in mobility data that will aid the research community. The project [factsheet](https://www.its.dot.gov/factsheets/pdf/Managing_ITS_DataPrivacyRisk.pdf) contains more information. You can become involved in this project in several
ways:

- **Pull the code and try it out!**  The [instructions](#introduction) that immediately follow will get you started.  [Build](#building-the-privacy-protection-application) instructions are also provided.  Let us know if you need help.
    - Github has [instructions](https://help.github.com/articles/signing-up-for-a-new-github-account) for setting up an account and getting started with repositories.
- If you would like to improve this code base or the documentation, [fork the project](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application#fork-destination-box) and submit a pull request (see the next section).
- If you find a problem with the code or the documentation, please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues/new).
- If the PPA does not solve your mobility data privacy problem, please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues/new) and prefix the issue title with [New Feature]. We would like to help.

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

- If you do not have one yet, create a personal (or organization) account on GitHub (assume your account name is `<your-github-account-name>`).
- Log into your personal (or organization) account.
- Fork [private-protection-application](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/fork) into your personal GitHub account.
- On your computer (local client), clone the master branch from you GitHub account :
```bash
$ git clone https://github.com/<your-github-account-name>/privacy-protection-appliation.git
```

### Additional Resources for Initial Setup
  
- [About Git Version Control](http://git-scm.com/book/en/v2/Getting-Started-About-Version-Control)
- [First-Time Git Setup](http://git-scm.com/book/en/Getting-Started-First-Time-Git-Setup)
- [Article on Forking](https://help.github.com/articles/fork-a-repo)

## Development in Your Fork

As an example, assume you are enhancing the existing code or documentation, or working on an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issues).  Complete the following steps on your computer (local client):

1. Add an upstream remote repository pointing to the Privacy Protection Application project repository.
    ```git remote add upstream https://github.com/usdot-its-jpo-data-portal/privacy-protection-application.git```  
1. Pull from the upstream remote repository to get the latest master branch.
    ```git pull --rebase upstream master```
1. Create a branch on your client (your forked repository).
    ```git branch enhancement```
   where ```enhancement``` is the name you want to use for the branch.
2. Switch to this branch
    ```git checkout enhancement```
3. Make the changes need to enhance the code. Note that before doing a pull request (below) you also want to ensure that any changes that have been accepted upstream can be integrated with any changes you are making now, so perform the following command in your forked repository branch prior to issuing a pull request:
    ```git pull upstream master```
4. When you have completed coding and integrating the upstream code, run the unit tests on your client in your build directory.
    TODO
   
1. When all the tests pass you can **add, commit, and push your updates to your `enchancement` branch in your fork.** Follow these steps:
    1. Check which files have been changed
        ```git status```
    1. Add the files that changed based on your enhancement, or that you want to include in the pull request.
       ```git add <file1> <file2>```
    1. Commit the files with a message describing the enhancement or bug fix.
       ```git commit -m "improved this aspect of the code."```
    1. Push the branch changes (enhancements) to your GitHub account:
       ```git push origin enhancement```

## Create A Pull Request

- After you have pushed your `enhancement` branch to your GitHub account, issue a pull request on GitHub. Details on how to perform a [pull request](https://help.github.com/articles/using-pull-requests) are on GitHub.
- If the pull request is closing an issue, include a comment with ```fixes #issue_number```.  This comment will close the issue when the pull request is merged.  For more information see [here](https://github.com/blog/1506-closing-issues-via-pull-requests).
- One of the main project developers will review your pull request and either merge it, or send you feedback. **Do not merge your own pull request.** Code review is essential. If you have not received feedback on your pull request in a timely fashion, contact us via email.
- Once your pull request has been reviewed and merged (possibly closing an issue), your enhancement will now be part of the privacy protection application project ```master``` branch.
- On your client machine, you can delete your branch.
  ```git branch -d enhancement```
- Pull from the privacy protection application project's ```master``` branch to have your changes reflected in your local (laptop/desktop) `master` branch:
```bash
$ git checkout master
$ git pull --rebase upstream master
```
- To include these in your fork's master branch, push them to your GitHub account.
  ```git push origin master```
- At this point the three master branches (one on organization, one on your GitHub account, and one on your client) are all in sync.

# Release Notes

## Hidden Markov Model Map Matching Release

### Motivation

- In general, use of map data improves the privacy protection algorithm for geolocation traces. However, inaccuarate map matching could lead to **under** suppression of potentially private trajectory data. Instances of this problem are typically seen when trip points are matched to wrong edge in an OSM motorway (US Interstate). The problem manifests when incorrectly mapped edges result in under counting intersection out degrees and therefore the under supression of trip points. With this in mind we decided to move towards a more robust and better established Hidden Markov Model map matching solution.

### Previous Version References

For help understanding the algorithm and terminology please refer to manual in the [master branch](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/blob/master/docs/cvdi-user-manual.md).

- [Error Correction](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/blob/master/docs/cvdi-user-manual.md#data-limitations-and-data-error-handling)
- [Map Fitting](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/blob/master/docs/cvdi-user-manual.md#map-preprocessing)
- [Critical Intervals](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/blob/master/docs/cvdi-user-manual.md#critical-interval-metrics)
- [Privacy Intervals](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/blob/master/docs/cvdi-user-manual.md#privacy-interval-metrics)

For help with the HMM map matching check out the section in [barefoot's wiki](https://github.com/bmwcarit/barefoot/wiki#hmm-map-matching) from which this release is based.

### Changes
- Updated release using the Hidden Markov Model (HMM) map matching. 
- This version has no GUI and only runs as [docker](https://www.docker.com/) containers. 
- The HMM map matching implementation is a C++ port of the wonderful [barefoot](https://github.com/bmwcarit/barefoot) project. 
    - The postgis container is the same as the one used by barefoot.  
- This version makes the changes to the original algorithm.
    - Replaces "explicit map fitting" with Hidden Markov Model map matching.
        - Trip points are matched to the best sequence of OSM ways for the entire trajectory, or sub-trajectory in the case HMM breaks.
        - Trip points that are unable to be matched are still "implicitly" matched using the original method.
        - Trip points are still checked for inclusion within explicit areas. These areas are created from the matched OSM ways using the same parameters as the original map fitting method.
    - Critical interval and privacy interval discovery is unchanged from the original algorithm.
    - In our previous map matching algorithm, a GPS error correction algorithm was used. The GPS error correction has been removed, since the HMM will handle these error better.

Aside from the changes listed here, most of the PPA functionality and configuration parameters are the same as the original. 

# Dependancies and Prerequisites

- Install [Git](https://git-scm.com/) or you will not be able to interact with this repository.
- Install [Docker](https://www.docker.com/) to build the images and run the containers.
- **Note**: The containers and included bash scripts have only been tested on Linux.

To build the required docker images:

```bash
./build_postgis.sh
./build_ppa.sh
```

# Building the OSM CSV Network File.

Start the postgis container.

```bash
./run_postgis.sh <database_name> <user> <password>
```

- _database_name_ The name for postgres database to be created.
- _user_ The name of the postgres user for the database.
- _password_ The password for the user.

Use ```docker ps``` or ```docker logs ppa_postgis``` to check the status of the container.

Download an OSM PBF. For larger datasets you can try [geofabrik](http://download.geofabrik.de/). Import the PBF to the database.

```bash
./map/host_import.sh <pbf_file>
```

- _pbf_file_ is the OSM PBF file

This will take serveral minutes, depending on the size of the file. Once completed, "Done." should appear. **Note** you may need to terminate the container process with ctrl-c.

Convert the database to an OSM CSV network file.

```bash
./run_osm_convert.sh <postgres_address> <postgres_port> <database_name> <user> <password> <osm_road_config> <output_file>
```

- _postgres_address_ The IP address of the container running postgres.
- _postgres_port_ The port of the container (usually 5432).
- _database_name_ The name of the postgres database.
- _user_ The name of the postgres user for the database.
- _password_ The name of the postgres password for the database.
- _osm_road_config_ The JSON file containing OSM way information used in the privacy protection algorithm. Unless there is a specific need to do otherwise, the unmodified ```example-data/config/road_config.json``` should be used.
- _output_file_ The host filesystem location where the OSM CSV network file should be stored.

If multiple network files are needed, re-run ```map/host_import.sh``` and ```run_osm_convert.sh``` using different OSM PBF files.

# Running the Privacy Protection Algorithm (PPA) Container

De-identify trajectory data within the PPA container.

```bash
./run_ppa.sh <batch_file> <osm_file> <ppa_config_file> <output_dir> <n_threads>
```

- _batch_file_ A text file that contains the absolute path to a trip file on each line.
- _osm_file_ The OSM CSV network file.
- _ppa_config_file_ A simple INI file containing PPA configuration options described in the [configuration section](#ppa-configuration-file).
- _output_dir_ A host filesystem directory where the PPA output will be stored.
- _n_threads_ The number of threads to use in the PPA. This is an upper bound, and the algorithm will adjust this number based on the available cores in the container.

The de-identificaiton of output is copied to _output_dir_ once the container exits. The directory has the following format:

- _info.log_ The normal runtime log for the PPA.
- _error.log_ The error log for the PPA.
- _[TRAJECTORY_ID].di.csv_ The de-identified trajectory files.
- _[TRAJECTORY_ID].kml_ The KML files for each de-identified trajectory. These files are only present if the _plot_kml_ option is set in the PPA configuration. 
- _[TRAJECTORY_ID].mm_ The map matching files for each trajectory. These are CSV files mapping the trip point indices of the trajectory to a OSM way IDs. These files are only present if the _save_mm_ option is set in the PPA configuration.

## PPA Configuration File

An example configuration file is included: ```example-data/config/ppa_config.ini```. This is simple key-value INI file. Values are interpreted as integers, doubles, or boolean flags (0 for false, anything else is interpreted as true). All values should be **positive or zero**. The fields are as follows:

- _save_mm_ [flag] Indicates the results of the map matching algorithm for each trajectory should be saved as a CSV file.
- _plot_kml_ [flag] Indicates the results of the PPA should be plotted and stored as a KML file for each trajectory.
- _count_points_ [flag] Indicates the number of processed points (including erroneous points and critical and privacy interval points) should be displayed to standard error.
- _af_fit_ext_ [double][meters] The fit extension for map matched fit areas.
- _af_toggle_scale_ [flag] Enable the scaling of fit areas.
- _af_scale_ [double][meters] The scaling factor used for the scaling of fit areas.
- _n_heading_groups_ [integer] The number of heading groups used for implicit map matching of segments of the trajectory that could not be map matched to the OSM road network. This dictates the degree change in the trajectory required for a new implicit edge.
- _min_edge_trip_points_ [integer] is the minimum number of trip points needed for the creation of an implicit edge.
- _ta_max_q_size_ [integer] The maximum number of edges that a sub-trajectory can traverse to be conisdered part of a turnaround critical interval.
- _ta_area_width_ [double][meters] The width of the implicit edges used to find turnaround critical intervals.
- _ta_heading_delta_ [double][degrees] The required change in heading degrees when a sub-trajectory exits and re-enters a fit edge area. If the heading change is greater than or equal to this value, then it is considered a turnaround critical interval. 
- _ta_max_speed_ [double][meters per second] The upper bound for how fast a sub-trajectory can be going to be considered part of a turnaround critical interval.
- _stop_min_distance_ [double][meters] The required distance a sub-trajectory must travel to be considered part of a stop critical interval.
- _stop_max_time_ [double][seconds] The upper bound for how long sub-trajectory can be stopped  (or stopping) to be considered part of a stop critical interval.
- _stop_max_speed_ [double][meters per second] The upper bound for how fast a sub-trajectory can be traveling to be considered part of stop critical interval.
- _min_direct_distance_ [double][meters] The lower bound on direct distnace used in finding privacy intervals.
- _min_manhattan_distance_ [double][meters] The lower bound on Manhattan distnace used in finding privacy intervals.
- _min_out_degree_ [integer] The lower bound on out degree used in finding privacy intervals.
- _max_direct_distance_ [double][meters] The upper bound on direct distance used in finding privacy intervals.
- _max_manhattan_distance_ [double][meters] The upper bound on Manhattan distnace used in finding privacy intervals.
- _max_out_degree_ [integer] The upper bound on out degree used in finding privacy intervals.
- _rand_direct_distance_ [double] The direct distance randomness factor added to privacy intervals.
- _rand_manhattan_distance_ [double] The Manhattan distance randomness factor added to privacy intervals.
- _rand_out_degree_ [double] The out degree randomness factor added to privacy intervals.
- _kml_stride_ [integer] The number of trajectory points to "stride over" before plotting the a new point in the KML. The higher this value is the easier the KML will be to render (at the expense of fidelity). This only applies if _plot_kml_ is set.
- _kml_suppress_di_ [flag] Indicates the de-identified portion of the trajectory should be excluded from the KML. The de-identified portion contains information about critical and privacy intervals, making it useful for debugging. This only applies if _plot_kml_ is set.

# Running Unit Tests

Test data is automatically added to to the PPA image. Use the following to run the unit tests: 

```bash
./run_test.sh
```

The tests cover most of the PPA funtionality. Currently, the postgis, multi-threading, and configuration code is **not** covered.

# Issues and Questions

If you need to contact the principal investigator or developers for this project, 
please submit an [issue](https://github.com/usdot-its-jpo-data-portal/privacy-protection-application/issue).  
Please consult the project [factsheet](https://www.its.dot.gov/factsheets/pdf/Managing_ITS_DataPrivacyRisk.pdf) for additional information and contacts.

