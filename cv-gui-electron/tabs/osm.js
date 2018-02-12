/*******************************************************************************
 * Copyright 2018 UT-Battelle, LLC
 * All rights reserved
 * Route Sanitizer, version 0.9
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For issues, question, and comments, please submit a issue via GitHub.
 *******************************************************************************/
'use strict';

/**
 * Converts contents of an OSM file to a quad file.
 */
var QuadConverter = (function() {
    /**
     * The filesytem module for reading kml files.
     */
    const fs = require('fs');

    /**
     * Parser for OSM (XML) file.
     */
    var parser = sax.parser(true);

    /**
     * Flag indicating the converter is still processing OSM nodes.
     */
    var findingNodes = true;

    /**
     * Hash of quad lines already processed. Key is a string consisting of the
     * latitude, longitude of the segment ends, as well as, the OSM way type.
     */
    var lines = {};

    /** 
     * Hash of OSM nodes already processed. Key == ID
     */
    var nodes = {};
    
    /** 
     * List of nodes references for the currently processed OSM way.
     */
    var wayRefNodes = [];

    /** 
     * Type of the currently processed OSM way.
     */ 
    var wayType = '';

    /**
     * ID of the currently processed OSM way.
     */
    var wayID = undefined;

    /**
     * Contents of the quad file. Initialized to the header.
     */
    var quadData = 'type,id,geography,attributes\n';

    /** 
     * Number of lines in the quad file.
     */
    var nLines = 0;

    /**
     * Path of the quad file.
     */
    var quadFile = undefined;

    /**
     * Southwest latitude boundary.
     */
    var swLat = undefined;

    /**
     * Southwest longitude boundary.
     */
    var swLng = undefined;

    /**
     * Northeast latitude boundary.
     */
    var neLat = undefined;

    /**
     * Northeast longitude boundary.
     */
    var neLng = undefined;

    /**
     * Flag indicating a parser error.
     */
    var isErr = false;

    /**
     * Return True if the given coordinates are within the configuration
     * bounds.
     *
     * @param {Number} lat The decimal latitude of the coordinate.
     * @param {Number} lng The decimal longitude of the coordinate.
     * @return {Boolean} True if the coordinates are in the bounds.
     */
    function isWithinBounds(lat, lng)  {
        return (lat >= swLat && lng >= swLng && lat <= neLat && lng <= neLng);
    }
    
    /**
     * Handle an OSM node's attributes. Extract the latitude and longitude, as
     * well as, the ID.
     * 
     * @param {Object} attrs The OSM node attributes.
     */
    function handleNode(attrs) {
        var latStr = attrs['lat'];
        var lngStr = attrs['lon'];
        var id = attrs['id'];
        var key = latStr + ':' + lngStr;

        if (nodes[key] == undefined) {
            nodes[key] = [];
        }                    

        nodes[key].push({'lat': Number(latStr), 'lng': Number(lngStr), 'id': Number(id)});
    }

    /**
     * Becuase node ID's might not be unique, use the lat/lng key to create 
     * to ensure uniqueness.
     */
    function setUniqueNodes() {
        var node;
        var uniqueNodes = {};

        for (node in nodes) {
            var nodeList = nodes[node];
            var ids = [];
            var i;
            var uniqueNode = nodeList[0];
            var uniqueID = uniqueNode['id'];
    
            ids.push(uniqueID);  
            uniqueNodes[uniqueID] = uniqueNode;
    
            for (i = 1; i < nodeList.length; i++) {
                uniqueID = nodeList[i]['id'];
                uniqueNodes[uniqueID] = uniqueNode;
            }
        }

        nodes = uniqueNodes;
    }
    
    /**
     * Handle the closing of an OSM way tag. Creates a quad line for each way
     * segment. Checks for duplicate entries using hashmap.
     */
    function handleWayClose() {
        var wayNodes = [];
        var refNode;
        var i,j;
        var a, b;
        var k1, k2;

        for (refNode in wayRefNodes) {
            wayNodes.push(nodes[wayRefNodes[refNode]]);
        }

        for (i = 0, j = 1; j < wayNodes.length; i++, j++) {
            a = wayNodes[i];
            b = wayNodes[j];

            if (!a || !b) {
                continue;
            }

            var aID = a['id'];
            var aLat = a['lat'];
            var aLng = a['lng'];
            var bID = b['id'];
            var bLat = b['lat'];
            var bLng = b['lng'];
   
            // Make two keys to check for duplicate segements, but in a 
            // order. 
            k1 = aLat + ':' + aLng + ':' + bLat + ':' + bLng + ':' + wayType;
            k2 = bLat + ':' + bLng + ':' + aLat + ':' + aLng + ':' + wayType;

            // We are only adding one line, so just check the first part of the
            // segment. All segment nodes will eventually be checked.
            if (!isWithinBounds(aLat, aLng)) {
                continue;
            }
    
            // Used both keys to check for duplicates.
            if (lines[k1] == undefined && lines[k2] == undefined) {
                quadData += 'line,' + nLines + ',' + aID + ';' + aLat + ';' + aLng + ':' + bID + ';' + bLat + ';' + bLng + ',way_type=' + wayType + ':way_id=' + wayID + '\n';
                lines[k1] = 1;
                nLines++;
            }
        }
    }

    /**
     * Do the OSM to quad conversion. 
     * 
     * @param {String} OSMFile Path of the OSM file.
     * @param {Function} progressCallback Progress callback function.
     * @param {Function} writeCallback Write callback function.
     * @param {Function} errorCallback Error callback function.
     */
    function convert(OSMFile, progressCallback, writeCallback, errorCallback) {
        // Initialize the progress.
        progressCallback(0);

        // Read the quad file and boundary from the configuration.
        DIModule.getQuadFile(function (quadFile_) {
            quadFile = quadFile_;
        });

        DIModule.getConfigVal('sw-lat', function(value, meta) {
            swLat = value;
        });

        DIModule.getConfigVal('sw-lng', function(value, meta) {
            swLng = value;
        });
    
        DIModule.getConfigVal('ne-lat', function(value, meta) {
            neLat = value;
        });

        DIModule.getConfigVal('ne-lng', function(value, meta) {
            neLng = value;
        });

        // Setup the parser.
        // Setup the handler for open tags.
        parser.onopentag = function (node) {
            // Errors should shutdown further parsing.
            if (isErr) {
                return;
            }

            // Only handle nodes and ways (and way sub fields)
            if (node.name == 'node') {
                handleNode(node.attributes);
            } else if (node.name == 'way') {
                // New way was found.
                // Check if this is the first way.
                if (findingNodes) {
                    setUniqueNodes();
                    findingNodes = false;
                    wayRefNodes = [];
                    wayType = '';
                }

                wayID = node.attributes['id'];
            } else if (node.name == 'nd') {
                // Way node refenence.
                wayRefNodes.push(Number(node.attributes['ref']))
            } else if (node.name == 'tag') {
                // Way highway type.
                if (node.attributes['k'] == 'highway') {
                    wayType = node.attributes['v'];
                }
            }
        };

        // Setup the handler for close tags.
        parser.onclosetag = function (tagName) { 
            // Errors should shutdown further parsing.
            if (isErr) {
                return;
            }

            // Only need to worry about way closings. 
            // This indicates a new line in the quad file.
            if (tagName == 'way') {
                handleWayClose();
                wayRefNodes = [];
                wayType = '';
            }
        };

        // Setup the error handler.
        parser.onerror = function (evt) {
            errorCallback("OSM file error: " + evt);
            isErr = true;
        };

        // Read the file.
        fs.readFile(OSMFile, 'utf8', function (err, data) {
            if (err) {
                errorCallback('Conversion file error on read: ' + err);
        
                return;
            }

            // Parse the OSM file.
            parser.write(data);

            // If the parser failed, don't do the write.
            if (isErr) {
                progressCallback(100);

                return;
            }

            // Check if no lines are found.
            if (nLines == 0) {
                errorCallback('OSM conversion produced no quad lines.');
            } 

            fs.writeFile(quadFile, quadData, 'utf8', function (err) {
                if (err) {
                    progressCallback(100);
                    errorCallback('Conversion file error on write: ' + err);

                    return;
                } 
            });

            // Upon successful conversion,
            DIModule.setQuadFile(quadFile);
            writeCallback(quadFile, nLines);
            progressCallback(100);
        }); 
    }

    return {
        convert: convert
    }
} ());

/**
 * Converts contents of an OSM file to a quad file.
 */
var OSMBoundFinder = (function() {
    /**
     * The filesytem module for reading kml files.
     */
    const fs = require('fs');

    /**
     * Parser for OSM (XML) file.
     */
    var parser = sax.parser(true);

    /**
     * The minimum southwest latitude as decimal degrees.
     */
    var minSWLat = undefined;

    /**
     * The minimum southwest longitude as decimal degrees.
     */
    var minSWLng = undefined;

    /**
     * The maximum northeast latitude as decimal degrees.
     */
    var maxNELat = undefined;

    /**
     * The maximum northeast longitude as decimal degrees.
     */
    var maxNELng = undefined;

    /**
     * Flag indicating a parser error.
     */
    var isErr = false;

    /**
     * Number of nodes compared.
     */
    var nNodes = 0;

    /**
     * Handle an OSM node's attributes. Update the minimum and maximum 
     * coordinates.
     * 
     * @param {Object} attrs The OSM node attributes.
     */
    function handleNode(attrs) {
        var lat = Number(attrs["lat"]);
        var lng = Number(attrs["lon"]);

        nNodes += 1;

        // Handle lamda case.
        if (minSWLat == undefined) {
            minSWLat = lat;
            maxNELat = lat;
            minSWLng = lng;
            maxNELng = lng;

            return;
        } 

        // Check for new min/max.
        if (lat < minSWLat) {
            minSWLat = lat;
        }
    
        if (lat > maxNELat) {
            maxNELat = lat;
        }

        if (lng < minSWLng) {
            minSWLng = lng;
        }
    
        if (lng > maxNELng) {
            maxNELng = lng;
        }
    }
    
    /**
     * Find the min/max bounds for the given OSMFile. The bounds are saved in 
     * the configuration as the southwest and northeast coordinates.
     *
     * @param {String} OSMFile Path of the OSM file.
     * @param {Function} progressCallback Progress callback function.
     * @param {Function} completeCallback Progress callback function.
     * @param {Function} errorCallback Error callback function.
     */
    function findBounds(OSMFile, progressCallback, completeCallback, errorCallback) {
        // Initialize the progress.
        progressCallback(0);

        // Setup the parser.
        // Setup the handler for open tags.
        parser.onopentag = function (node) {
            // Errors should shutdown further parsing.
            if (isErr) {
                return;
            }

            // Only handle nodes.
            if (node.name == 'node') {
                handleNode(node.attributes);
            }
        };

        // Setup the handler for close tags.
        parser.onclosetag = function (tagName) { 
            // Errors should shutdown further parsing.
            if (isErr) {
                return;
            }
        };

        // Setup the error handler.
        parser.onerror = function (evt) {
            errorCallback('OSM file error: ' + evt);
            isErr = true;
        };

        // Read the file.
        fs.readFile(OSMFile, 'utf8', function (err, data) {
            if (err) {
                errorCallback('Error finding bounds. File error on read: ' + err);
        
                return;
            }

            // Parse the OSM file.
            parser.write(data);

            // If the parser failed, don't do the write.
            if (isErr) {
                progressCallback(100);
                return;
            }

            if (!minSWLat || !minSWLng || !maxNELat || !maxNELng) {
                progressCallback(100);
                errorCallback('Error finding bounds. OSM file has no valid nodes.');

                return;
            }

            var newConfig = {
                'sw-lat': minSWLat,
                'sw-lng': minSWLng,
                'ne-lat': maxNELat,
                'ne-lng': maxNELng
            }

            // Upon successful conversion,
            DIModule.setNewConfig(newConfig, function (errStr) {
                if (errStr) {
                    errorCallback('Error finding bounds: ' + errStr);
    
                    return;
                }
                
                progressCallback(100);
                completeCallback(nNodes);
            });
        }); 
    }

    return {
        findBounds: findBounds
    }
} ());

/**
 * The OSM tab allows the user to create and save a ".quad" file. The options
 * here are maintained be the DI module's configuration. The are read when the 
 * tab contents are loaded. If a quad file is created in this tab it is saved
 * to the configuration tab.
 */
GUI.TABS.osm = (function() {
    /**
     * The remote dialog for opening files a directory.
     */
    const {dialog} = require('electron').remote

    /**
     * Path of the OSM file.
     */
    var OSMFile = '';
   
    /**
     * Return True if the conversion is possible. The converison is possible       
     * if the quad file and OSM file are both selected. 
     */
    function isReady() {
        var quadFile_;
    
        DIModule.getQuadFile(function (quadFile) {
            quadFile_ = quadFile;
        });

        return OSMFile && quadFile_;
    }

    /**
     * Set the quad bounds in the GUI from the DIModule configuration.
     */
    function setBounds() {
        // Load the quad bounds from the configuration.
        var boudaryProps = ['sw-lat', 'sw-lng', 'ne-lat', 'ne-lng'];

        for (var i = 0; i < boudaryProps.length; i++) {
            var prop = boudaryProps[i];

            // Load in the values, mins, max, and steps.
            DIModule.getConfigVal(prop, function(value, meta, errStr) {
                if (errStr) {
                    GUI.log('[OSM] Internal Error: ' + errStr, 2);
                }  

                var elem = $('input[name="' + prop + '"]');

                // Set the element value.
                if (meta.type == 'float' && !countDecimals(value)) {
                    // If it's a float with no decimal places, add one 
                    // decimal place to the format.
                    elem.val(parseFloat(value).toFixed(1));
                } else {
                    elem.val(value);
                }

                // Also set the min, max, and step to prevent the user from
                // exceeding any bounds via the arrows (does not apply to 
                // direct entry).
                if (meta.min) {
                    elem.attr('min', meta.min);
                } 

                if (meta.max) {
                    elem.attr('max', meta.max);
                } 

                if (meta.step) {
                    elem.attr('step', meta.step);
                } 
            });
        }
    }

    /**
     * Initialize the tab. Load the HTML contents and any saved state.
     * 
     * @param {Function} callback The callback to call when the content is 
     *                            loaded.
     */
    function initialize(callback) {
        // All initialize functions must change the active tab.
        if (GUI.activeTab != 'osm') {
            GUI.activeTab = 'osm';
        }

        // Load the HTML content. 
        $('#content').load("./tabs/osm.html", function () {
            // Add the localized text to the elements.
            localize();

            // Inform the GUI that the content is reeady.
            // This loads tool tips and transforms the Switchery elements for
            // for the tab.
            GUI.contentReady(callback);

            // Load in the quad file from the config.
            DIModule.getQuadFile(function (quadFile) {
                if (quadFile) {
                    $('div.quad_file').text(quadFile);
                } else {
                    $('div.quad_file').text('None');
                }
            });

            // Set the OSMFile.
            if (OSMFile) {
                $('div.osm_file').text(OSMFile);
            } else {
                $('div.osm_file').text('None');
            }

            // Set the configured bounds.
            setBounds();

            // Check if bound finding is possible.
            // Disable/enable the bound finding appropriately.
            if (OSMFile) {
                $('.find_bounds').removeClass('disabled');
            } else {
                $('.find_bounds').addClass('disabled');
            }
 
            // Check if the converison is possible.
            // Disable/enable the convert button appropriately.
            if (isReady()) {
                $('.make_quad').removeClass('disabled');
            } else {
                $('.make_quad').addClass('disabled');
            }

            // UI Hooks
            // Handle the user selecting the OSM input file.
            $('a.osm_load_btn').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                // Open the OSM file dialog.
                dialog.showOpenDialog({properties: ['openFile']}, function (paths) {
                    // Check for cancellation.
                    if (paths == undefined) {
                        OSMFile = '';
                        $('div.osm_file').text('None'); 
                    } else {
                        OSMFile = paths[0];
                        $('div.osm_file').text(paths[0]);
                    }

                    // Check if bounds finding is possible.
                    if (OSMFile) {
                        $('.find_bounds').removeClass('disabled');
                    } else {
                        $('.find_bounds').addClass('disabled');
                    }

                    // Check if converison is possible.
                    if (isReady()) {
                        $('.make_quad').removeClass('disabled');
                    } else {
                        $('.make_quad').addClass('disabled');
                    }
                });
            });

            // Handle the user selecting the quad output file.
            $('a.osm_quad_btn').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                var quadFile_;

                // Load in the quad file from the config.
                DIModule.getQuadFile(function (quadFile) {
                    quadFile_ = quadFile;
                });

                // Open the quad file dialog.
                dialog.showSaveDialog({defaultPath: quadFile_}, function (filename) { 
                    // Check for cancellation.
                    if (filename == undefined) {
                        $('div.quad_file').text(quadFile_);
                    } else {
                        var path = filename;

                        $('div.quad_file').text(path);
                        DIModule.setQuadFile(path);
                    }

                    // Check if converison is possible.
                    if (isReady()) {
                        $('.make_quad').removeClass('disabled');
                    } else {
                        $('.make_quad').addClass('disabled');
                    }
                });

            });

            // Handle the user changing one of the boundary options. Passes the
            // new value to the DIModule to validate. If the new value is 
            // invalid, the error is reported.
            $('input[type="number"]').change(function () {
                var element = $(this);
                var name = element.attr("name");
                var val = element.val();

                DIModule.setConfigVal(name, val, function(newVal, errStr) {
                    if (errStr) {
                        dialog.showErrorBox('Configuration Error', name + ': ' + errStr);
                        element.val(newVal);

                        return;
                    }
                });
            });

            // Handle the user pressing the find OSM bounds button. Load the
            // OSM and find the binds.
            $('.find_bounds').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();
               
                GUI.log("[OSM] Finding bound for OSM file...");
                OSMBoundFinder.findBounds(OSMFile, function(progress) {
                    $('.convert_progress_fill').width(progress.toString() + '%');
                }, function(nNodes) {
                    GUI.log('[OSM] Found bounds for OSM file: ' + OSMFile + ' with ' + nNodes.toString() + ' nodes.');
                    setBounds();
                }, function(err) {
                    GUI.log('[OSM] ' + err, 2); 
                    dialog.showErrorBox('OSM Bound Finding Error', err);
                });
            });

            // Handle the user pressing the convert button. Load the OSM file
            // and convert it to a quad file.
            $('.make_quad').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();
               
                GUI.log("[OSM] Converting OSM file to quad...");
                QuadConverter.convert(OSMFile, function(progress) {
                    $('.convert_progress_fill').width(progress.toString() + '%');
                }, function(quadFile, nLines) {
                    GUI.log('[OSM] OSM file conversion complete.'); 
                    GUI.log('[OSM] Wrote quad file: ' + quadFile + ' with ' + nLines.toString() + ' lines.'); 
                }, function(err) {
                    GUI.log('[OSM] ' + err, 2); 
                    dialog.showErrorBox('OSM Conversion Error', err);
                });
            });
        });
    }

    /**
     * This function is called when the tab is switched.
     * 
     * @param {Function} callback Callback called when cleanup is called.
     */
    function cleanup(callback) {
        if (callback) callback();
    }

    return {
        initialize: initialize,
        cleanup: cleanup      
    }
} ());
