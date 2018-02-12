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
 * Contains shared methods for the configuration and advanced configuration.
 */
var SharedConfig = (function() {
    /**
     * The remote dialog for opening files a directory.
     */
    const {dialog} = require('electron').remote

    /**
     * The filesytem module for reading kml files.
     */
    const fs = require('fs');

    /**
     * Enable the mapfit scale option in the GUI.
     */
    function enableScaleFactor() {
        $('input[name="mapfit-scale"]').removeClass('disabled');
        $('input[name="mapfit-scale"]').removeAttr('disabled');
    }

    /**
     * Disable the mapfit scale option in the GUI.
     */
    function disableScaleFactor() {
        $('input[name="mapfit-scale"]').addClass('disabled');
        $('input[name="mapfit-scale"]').attr('disabled', 'disabled');
    }

    /**
     * Update the GUI fields based off the values of the configuration object.
     * Because the DI module configuration values are assumed to be correct, 
     * there is no need to check them in this function. 
     */    
    function updateConfig() {
        // Load in the values, mins, max, and steps.
        DIModule.getConfig(function(prop) {
            // Ignore OSM config values.
            if (prop == 'sw-lat' || prop == 'sw-lng' || prop == 'ne-lat' || prop == 'ne-lng') {
                return;
            }

            DIModule.getConfigVal(prop, function(value, meta, errStr) {
                if (errStr) {
                    GUI.log('[Configuration] Internal Error: ' + errStr, 2);

                    return;
                }  

                // Get the element to for the configuration property.
                var elem = $('input[name="' + prop + '"]');

                // Check for the special case of boolean types.
                if (meta.type == 'boolean') {
                    elem.prop('checked', value);
                    elem.change();
    
                    // Check for the other special case of the map fit scale
                    // being set.
                    if (prop == 'mapfit-scale-toggle') {
                        if (value) {
                            enableScaleFactor();
                        } else {
                            disableScaleFactor();
                        }  
                    }

                    return;
                } 
                  
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
        });
    }

    /**
     * Handle the user's change to the configuration. Passes the new value to 
     * DIModule to validate. If the new value is invalid, then the error is 
     * reported the log.
     * 
     * @param {Object} element The GUI configuration element that was changed.
     */
    function handleConfigChange(element) {
        var name = element.attr('name');
        
        var meta;
        var val;
            
        DIModule.getConfigMeta(name, function(meta_, errStr) {
            if (errStr) {
                GUI.log('[Configuration] Internal Error: ' + errStr, 2);

                return;
            }

            meta = meta_; 
        });

        if (meta.type == 'boolean') {
            val = element.prop('checked');
        } else {    
            val = element.val();
        }
    
        // Set the value. If the errStr is set then report the error to the 
        // log.
        DIModule.setConfigVal(name, val, function(newVal, errStr) {
            if (errStr) {
                dialog.showErrorBox('Configuration Error: ', errStr);
                element.val(newVal);

                return;
            } 

            if (meta.type == 'boolean') {
                // Disable this element if mapfit scale is set to false.
                if (name == 'mapfit-scale-toggle') {
                    if (newVal) {
                        enableScaleFactor();
                    } else {
                        disableScaleFactor();
                    }
                }
            }
        });
    }

    /**
     * Initialize the shared elements of configuration page.
     */
    function initElements() {
        // Hook the text inputs.
        $('input[type="text"]').change(function() {
            handleConfigChange($(this));
        });

        // Hook the number inputs.
        $('input[type="number"]').change(function() {
            handleConfigChange($(this));
        });

        // Hook the checkbox inputs.
        $('input[type="checkbox"]').change(function() {
            handleConfigChange($(this));
        });

        // Hook the load configuration button.
        $('.load').click(function (e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            // This will open a file for reading. It will try to convert
            // the contents of the file to an object. Once the object is
            // created it will be loaded into the correct elements,
            // then checked for validity.
            dialog.showOpenDialog({properties: ['openFile']}, function (paths) {
                // Check if user cancelled.
                if (paths == undefined) {
                    GUI.log("[Configuration] Failed to load configuration file: User cancelled.", 2);

                    return;
                }
    
                var path = paths[0];
                var json = "";

                // Read and store the contents of the file in the 
                // json string.
                fs.readFile(path, 'utf8', function (err, data) {
                    if (err) {
                        dialog.showErrorBox('Configuration File Error', err);

                        return;
                    }

                    // When the file is done loading convert the string 
                    // to an object.
                    try {
                        var userConfig = JSON.parse(data);
                    } catch(e) {
                        // Conversion failed.
                        dialog.showErrorBox('Configuration File Error', e.toString());
                        
                        return;
                    }

                    DIModule.setNewConfig(userConfig, function (errStr) {
                        if (errStr) {
                            // Get the text of the element.
                            dialog.showErrorBox('Configuration File Error', errStr);
            
                            return;
                        }
    
                        // Upon completion of setting the new config update
                        // config in the GUI.
                        updateConfig();
                    });
                });
            });
        });
        
        // Hook the save configuration button.
        $('.save').click(function (e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            // When the file is opened, a writer is created.
            // Create a new object from the stored
            // configuration. This object is converted to a JSON string
            // and saved to a file.
            dialog.showSaveDialog(function (filename) {
                if (filename == undefined) {
                    GUI.log('[Configuration] Failed to create configuration file: User cancelled.', 2);

                    return;
                }

                // Object to be saved.
                var configOut = {};

                // Fill the object from the DI module.
                DIModule.getConfig(function (prop) {
                    DIModule.getConfigVal(prop, function(value, meta) {
                        configOut[prop] = value;
                    });
                });

                fs.writeFile(filename, JSON.stringify(configOut, null, ' '), function (err) {
                    // Check for write error.
                    if (err) {
                        GUI.log('[Configuration] Error writing configuration file: ' + err, 2);
                    }

                    GUI.log('[Configuration] Wrote config file: ' + filename);
                });
            });
        });
    }

    return {
        updateConfig: updateConfig,
        initElements: initElements
    }
} ());
