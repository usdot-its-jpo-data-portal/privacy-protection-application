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
 * The map tab provides an interface into the Google Map iframe. It allows file
 * content to passed to the map.
 */
GUI.TABS.map = (function() {
    /**
     * The remote dialog for opening files a directory.
     */
    const {dialog} = require('electron').remote

    /**
     * The filesytem module for reading kml files.
     */
    const fs = require('fs');

    /**
     * Flag indicating the map content is loading.
     */
    var isLoading = false;

    /**
     * Get the Google bounds from the bounds object.
     * 
     * @param {Object} bounds The boundary coordiantes.
     * @return {Object} The bounds as literal object for Google's API.
     */
    function googleBounds(bounds) {
        return {'east': bounds['ne-lng'], 'north': bounds['ne-lat'], 'south': bounds['sw-lat'], 'west': bounds['sw-lng']}
    }

    /**
     * Center the map by getting the OSM bounds and sending the to Google Maps
     * iframe. The frame script will make Google API calls.
     */
    function centerMap() {
        // Get the map bounds from the config.                
        var bounds = {'sw-lat': undefined, 'sw-lng': undefined, 'ne-lat': undefined, 'ne-lng': undefined};

        for (var prop in bounds) {
            // Load in the values, mins, max, and steps.
            DIModule.getConfigVal(prop, function(value, meta, errStr) {
                if (errStr) {
                    GUI.log('[Map] Internal Error: ' + errStr, 2);
                }  

                bounds[prop] = value;
            });
        }

        // Get the frame.
        var frame = document.getElementById('map');
            
        var gBounds = googleBounds(bounds);         

        GUI.log('[Map] Centering map with OSM bounds.');

        // Construct a message to send to the iframe.
        var message = {
            type: 'center',
            bounds: gBounds
        };
    
        // Flag the frame a loading. 
        isLoading = true;
        
        // Display the data loading screen.
        $('#cache .data-loading').clone().appendTo($('.content_wrapper'));
        
        // Hide the frame.
        frame.style.visibility = 'hidden';
        
        // Post the file content to the frame.
        frame.contentWindow.postMessage(message, '*');
    }

    /**
     * Initialize the tab. Load the HTML contents and any saved state.
     * 
     * @param {Function} callback The callback to call when the content is 
     *                            loaded.
     */
    function initialize(callback) {
        // All initialize functions must change the active tab.
        if (GUI.activeTab != 'map') {
            GUI.activeTab = 'map';
        }

        // Load the HTML content.
        $('#content').load("./tabs/map.html", function () {
            localize();

            // Inform the GUI that the content is reeady.
            // This loads tool tips and transforms the Switchery elements for
            // for the tab.
            GUI.contentReady(callback);

            // Listen for message events from the iframe. The only thing we 
            // looking for is confirmation that the data is done loading.
            window.addEventListener('message', function (e) {
                if (!isLoading) {
                    return;
                }

                $('.content_wrapper .data-loading').remove();
                isLoading = false;
               
                document.getElementById('map').style.visibility = 'visible';
            });

            // Hook the load button.
            $('a.center_map').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();
            
                // Center the map.
                centerMap();
            });

            // Hook the load button.
            $('a.load_kml').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                // Open dialog to select a KML file. TODO non-kml.
                dialog.showOpenDialog({properties: ['openFile']}, function (paths) {
                    // Check for an error getting the file.
                    if (paths == undefined) {
                        GUI.log("[Map] Failed to load KML file: User cancelled.", 2);

                        return;
                    }

                    var kmlPath = paths[0];

                    GUI.log("[Map] Loading KML file: " + kmlPath);

                    // Read the file. 
                    // Pass the content of the file to the Google Map iframe.
                    fs.readFile(paths[0], 'utf8', function (err, data) {
                        // Check for file read error.
                        if (err) {
                            GUI.log("[Map] Failed to load KML file: " + err, 2)                            

                            return;
                        }

                        // Get the frame.
                        var frame = document.getElementById('map');

                        // Construct a message to send to the iframe.
                        var message = {
                            type: 'KML',
                            KML: data
                        };
        
                        // Flag the frame a loading. 
                        isLoading = true;
                        
                        // Display the data loading screen.
                        $('#cache .data-loading').clone().appendTo($('.content_wrapper'));
                        
                        // Hide the frame.
                        frame.style.visibility = 'hidden';
                        
                        // Post the file content to the frame.
                        frame.contentWindow.postMessage(message, '*');
                    });
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
        // If the user switches tabs, the turn off the loading flag.
        isLoading = false;

        if (callback) callback();
    }

    return {
        initialize: initialize,
        cleanup: cleanup
    }
} ());
