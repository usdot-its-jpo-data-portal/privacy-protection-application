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
'use strict'

/**
 * The DIModule is the central object interfacing the GUI with the 
 * the de-identification (DI) module routine. It will preferably be the only 
 * object that contains the needed data to run the routine. All other 
 * components will be repsonsible for delvering user input to this module.
 * 
 * This object requires the GUI module to be intitialized.
 */
var DIModule = (function() {
    /**
     * The filesytem module. Used to check if the quad file has been altered.
     */
    const fs = require('fs');

    /**
     * Module to get the path of the CVDI module.
     */
    const path = require("path");

    /**
     * Module with connected to the C++ Connected Vehicle DeIdentification 
     * Routines.
     */
    const cvdiModule = require(path.join(__dirname, 'modules/cvdi_nm'))  

    /**
     * An array of input file paths.
     */
    var inputFiles = [];

    /**
     * The output directory path.
     */
    var outputDir = undefined;

    /**
     * The quad file path.
     */
    var quadFile = undefined;

    /**
     * The log file path.
     */
    var logFile = undefined;

    /**
     * The callback used when the run state is changed. This is allows the 
     * the GUI to be updated after the run state is changed.
     */
    var runStateCallback = undefined;

    /**
     * Stores the value of the run state.
     */
    var isRunnable = false;

    /**
     * Flag indicating the quad file should be rebuilt when the DI module is 
     * executed.
     */
    var reBuildQuad = true;

    /**
     * This is the configuration object. This is for fields that are not 
     * directly related to the filesystem.
     */
    var config = {
        'kml-output':                true,
        'lat-field':                 'Latitude',
        'lng-field':                 'Longitude',
        'heading-field':             'Heading',
        'speed-field':               'Speed',
        'time-field':                'Gentime',
        'uid-fields':                'RxDevice,FileId',
        'fit-ext':                   .5,
        'mapfit-scale-toggle':       false,
        'mapfit-scale':              1.0,
        'heading-groups':            36,
        'min-edge-trippoints':       10,
        'ta-max-q':                  20,
        'ta-area-width':             30.0,
        'ta-max-speed':              100.0,
        'ta-heading-delta':          90.0,
        'stop-max-time':             1.0,
        'stop-min-dist':             50.0,
        'stop-max-speed':            2.5,
        'min-direct-distance':       10.0,
        'max-direct-distance':       11000.0,
        'min-manhattan-distance':    10.0,
        'max-manhattan-distance':    11000.0,
        'min-out-degree':            0,
        'max-out-degree':            10,
        'rand-direct-distance':      0.0,
        'rand-manhattan-distance':   0.0,
        'rand-out-degree':           0.0,
        'sw-lat':                    35.946920,
        'sw-lng':                    -83.938486,
        'ne-lat':                    35.955526,
        'ne-lng':                    -83.926738
    };

    /**
     * Lookup function for meta data properties for each configured property.
     * Returns a meta object for the config.
     *
     * We need this function, as opposed to a static object, because loaded
     * configurations must be validated with their respective input values and 
     * not the valued already cached in this model.
     * 
     * @param {Object} config The configuration object to build the meta from.
     * @return {Object} An object containing the meta values for the 
     * confiuration.
     */
    function getConfigMetaObject(configIn) {
        return {
            'kml-output': {type: 'boolean'},
            'lat-field': {type: 'string'},
            'lng-field': {type: 'string'},
            'heading-field': {type: 'string'},
            'speed-field': {type: 'string'},
            'time-field': {type: 'string'},
            'uid-fields': {type: 'csv-string'},
            'fit-ext': {type: 'float', min: 0.0, step: 0.01},
            'mapfit-scale-toggle': {type: 'boolean'},
            'mapfit-scale': {type: 'float', min: 1.0, step: 0.01},
            'heading-groups': {type: 'integer', min: 12},
            'min-edge-trippoints': {type: 'integer', min: 0},
            'ta-max-q': {type: 'integer', min: 1},
            'ta-area-width': {type: 'float', min: 1.0, step: 0.01},
            'ta-max-speed': {type: 'float', min: 0.0, step: 0.01},
            'ta-heading-delta': {type: 'float', min: 45.0, max: 360.0, step: 0.01},
            'stop-max-time': {type: 'float', min: 1, step: 0.1},
            'stop-min-dist': {type: 'float', min: 1.0, step: 0.01},
            'stop-max-speed': {type: 'float', min: 0.0, step: 0.01},
            'min-direct-distance': {type: 'float', min: 0.0, max: configIn['max-direct-distance'], step: 0.01, maxProp: 'max-direct-distance'},
            'min-manhattan-distance': {type: 'float', min: 0.0, max: configIn['max-manhattan-distance'], step: 0.01, maxProp: 'max-manhattan-distance'},
            'min-out-degree': {type: 'integer', min: 0, max: configIn['max-out-degree'], maxProp: 'max-out-degree'},
            'max-direct-distance': {type: 'float', min: configIn['min-direct-distance'], step: 0.01, minProp: 'min-direct-distance'},
            'max-manhattan-distance': {type: 'float', min: configIn['min-manhattan-distance'], step: 0.01, minProp: 'min-manhattan-distance'},
            'max-out-degree': {type: 'integer', min: configIn['min-out-degree'], minProp: 'min-out-degree'},
            'rand-direct-distance': {type: 'float', min: 0.0, max: 1.0, step: 0.01},
            'rand-manhattan-distance': {type: 'float', min: 0.0, max: 1.0, step: 0.01},
            'rand-out-degree': {type: 'float', min: 0.0, max: 1.0, step: 0.01},
            'sw-lat': {type: 'float', min: -84.0, max: configIn['ne-lat'], step: 0.01, maxProp: 'ne-lat'},
            'sw-lng': {type: 'float', min: -180.0, max: configIn['ne-lng'], step: 0.01, maxProp: 'ne-lng'},
            'ne-lat': {type: 'float', min: configIn['sw-lat'], max: 84.0, step: 0.01, minProp: 'sw-lat'},
            'ne-lng': {type: 'float', min: configIn['sw-lng'], max: 180.0, step: 0.01, minProp: 'sw-lng'}
        }
    }

    /**
     * Handle the callback event from the DeIdentification node module.
     * 
     * @param {Object} evt The event from the de-identification algorithm.
     */
    function diCallback(evt) {
        if (evt.type === 'progress') {
            GUI.updateProgress(evt.progress * 100); 
        } else if (evt.type === 'message') {
            GUI.log(evt.message);
        } else if (evt.type === 'warning') {
            GUI.log(evt.message, 1);
        } else if (evt.type === 'error') {
            GUI.log(evt.message, 2);
        } else if (evt.type === 'debug') {
            console.log(evt);
        }
    }

    /**
     * Make the required output directories for de-identitication module.
     * Creates a di_out (and kml_out if KML output is enabled) in the output
     * directory.
     * 
     * @return {Boolean} True if directories could be succesfully created, 
     *                   false otherwise.
     */
    function makeOutputDirectories() {
        var filePath = path.join(outputDir, 'di_out');
        var stat;

        try {
            stat = fs.lstatSync(filePath);
            
            if (!stat.isDirectory()) {
                GUI.log('[DI] Could not create "di_out" directory. A file name "di_out" already exists.', 2);
            
                return false;
            }
        } catch (err) {
            try {
                fs.mkdirSync(filePath); 
            } catch (err) {
                GUI.log('[DI] Could not create "di_out" directory: ' + err, 2);

                return false;
            }
        }

        if (!config['kml-output']) {
            return true;
        }
        
        filePath = path.join(outputDir, 'kml_out');

        try {
            stat = fs.lstatSync(filePath);
            
            if (!stat.isDirectory()) {
                GUI.log('[DI] Could not create "kml_out" directory. A file name "kml_out" already exists.', 2);
            
                return false;
            }
        } catch (err) {
            try {
                fs.mkdirSync(filePath); 
            } catch (err) {
                GUI.log('[DI] Could not create "kml_out" directory: ' + err, 2);

                return false;
            }
        }

        return true;
    }

    /**
     * Send the required JSON object to the CVDI module, causing the DI
     * routine to be executed.
     *   
     * @param {Object} object A JSON object in the form:
     *      {
     *          config: CVDI JSON configuration,
     *          outputModuleFile: The module file for the output directory.
     *          quadModuleFile: The module file for the quad file.
     *          files: The input files
     *      } 
     */
    function runDI() {
        // Guard against running when the flag is not set.
        if (!isRunnable) {
            return;
        }

        var diObject = {
            config: config,
            outputDir: outputDir,
            quadFile: quadFile,
            buildQuad: reBuildQuad,
            logFile: logFile,
            files: inputFiles
        };

        // Try to make the output directories.
        if (!makeOutputDirectories()) {
            // Failed to make directories. Don't run.
            return;
        }

        // Only de-identify if the module is loaded.         
        if (cvdiModule) {
            GUI.log('Running de-identifaction routine. See "' + logFile + '" for log output.');
            cvdiModule.deIdentify(diObject, diCallback, function () {});
        } else {
            GUI.log('Could not perform de-identification: Module not loaded.', 2);
        }

        // After running, set the re-build flag to false to prevent rebuilding
        // the quad next run.
        reBuildQuad = false;
    }

    /**
     * Update the run state. Check if the other views have enough input in order
     * to run the DI routine. In order to run the DI routine, there needs to 
     * be a quad file, an output directory, a log file, and at least one input 
     * file.
     */
    function updateRunState() {
        isRunnable = outputDir != undefined && 
            quadFile != undefined && 
            logFile != undefined &&
            inputFiles.length > 0;

        // Guard against unset callback.
        if (DIModule.runStateCallback != undefined) {
            DIModule.runStateCallback(isRunnable);
        }
    }

    /**
     * Adds the input file to the end of the input file list. Updates the run
     * state.
     * 
     * @param {Object} moduleFile Module file object for the input file.
     * @param {string} path Path to the input file in the user's filesystem.
     */
    function addInputFile(path) {
        // Add the input file.
        inputFiles.push(path);

        // Update the run state.
        updateRunState();        
    }

    /**
     * Removes an input file from the input file list. Updates the run state.
     *
     * @param {number} index Index of the input file in list.
     */
    function removeInputFile(index) {
        // Guard against invalid indices.
        if (index >= inputFiles.length || index < 0) {
            return;
        }

        // Remove the file from the list.
        inputFiles.splice(index, 1);
        
        // Update the run state.
        updateRunState();        
    }

    /**
     * Allow to access to the input files.
     *
     * @param {Function} callback Called for each input file.
     */
    function getInputFiles(callback) {
        for (var i = 0; i < inputFiles.length; ++i) {
            callback(inputFiles[i]);
        }
    }

    /**
     * Set the output directory. Update the run state.
     *
     * @param {string} path Path to the output directory in the user's 
     *                      filesystem.
     */
    function setOutputDir(path) {
        outputDir = path;

        updateRunState();
    }

    /**
     * Set the quad file. Update the run state.
     *
     * @param {string} path Path to the quad file in the user's filesystem.
     */
    function setQuadFile(path) {
        quadFile = path;

        // Alwqys rebuild when the file is set.
        reBuildQuad = true;

        // Watch for modification of the quad file.
        fs.watchFile(quadFile, function(curr, prev) {
            if (curr.mtime != prev.mtime) {
                reBuildQuad = true;
            } else {
                reBuildQuad = false;
            }
        });

        updateRunState();
    }

    /**
     * Set the log file. Update the run state.
     *
     * @param {string} path Path to the log file in the user's filesystem.
     */
    function setLogFile(path) {
        logFile = path;

        updateRunState();
    }

    /**
     * Allow to access to the output directory.
     *
     * @param {Function} callback Called for the output directory.
     */
    function getOutputDir(callback) {
        callback(outputDir);
    }

    /**
     * Allow to access to the quad file.
     *
     * @param {Function} callback Called for the quad file.
     */
    function getQuadFile(callback) {
        callback(quadFile);
    }

    /**
     * Allow access to the log file.
     * 
     * @param {Function} callback Called for the log file.
     */
    function getLogFile(callback) {
        callback(logFile);
    }

    /**
     * Set a new configuration. Checks the values of the configuration, as well
     * as, the ranges.
     * 
     * This function checks values and then checks ranges. This prevents 
     * the checking ranges against the old configuration. However, if the 
     * the new configuration does not provide a value for property, then 
     * the old value is used in the check.
     * 
     * @param {Object} newConfig The new configuration.
     * @param {Function} callback Called when configuration is set or an
     *                            error occured.
     */
    function setNewConfig(newConfig, callback) {
        var metaObject = getConfigMetaObject(newConfig);
        var tmpConfig = {};
    
        // The first loop checks for correct types. For numeric types, it does
        // the conversion and checks for NaN.
        for (var prop in config) {
            // Check if the property is a config property.
            if (prop in newConfig) {
                // This is new config value. Check the type.
                var newVal = newConfig[prop];
                var meta = metaObject[prop];
    
                // Check the types. 
                if (meta.type == 'boolean') {
                    newVal = Boolean(newVal);
                } else if (meta.type == 'string') {
                    newVal = String(newVal);
    
                    if (newVal.length > 128) {
                        callback(prop + ': ' + 'field cannot exceed 128 characters'); 
    
                        return;
                    }
    
                    var commaIndex = newVal.indexOf(',');
    
                    if (commaIndex > -1) {
                        callback(prop + ': ' + 'field cannot contain commas'); 
    
                        return;
                    }
                } else if (meta.type == 'csv-string') {
                    newVal = String(newVal);
    
                    if (newVal.length > 128) {
                        callback(prop + ': ' + 'field cannot exceed 128 characters'); 
    
                        return;
                    } 
                } else if (meta.type == 'integer') {
                    newVal = Number(newVal);
    
                    if (isNaN(newVal)) {
                        callback(prop + ': ' + 'value is not a number'); 
    
                        return;
                    }
    
                    if (!Number.isInteger(newVal)) {
                        callback(prop + ': ' + 'value is not an integer');
    
                        return;
                    }
                } else if (meta.type == 'float') {
                    newVal = Number(newVal);

                    if (isNaN(newVal)) {
                        callback(prop + ': ' + 'value is not a number'); 
    
                        return;
                    }
                }
    
                // Set the new config value.
                newConfig[prop] = newVal;
            } else {
                // If the property is not provided, then load the current 
                // property in the tmp config. This allows for checking 
                // ranges of values.
                tmpConfig[prop] = config[prop];
            }
        }

        // The second loop adds the tmp config to the new config.
        for (var prop in tmpConfig) {
            newConfig[prop] = tmpConfig[prop];
        }
            
        // The third loop checks the numeric range for all numeric types. We 
        // have to wait to do this to avoid doing comparisons with incorrect 
        // types within the config.
        for (var prop in config) {
            var newVal = newConfig[prop];
            var meta = metaObject[prop];

            if (meta.type == 'integer' || meta.type == 'float') {
                if ('min' in meta && newVal < meta.min) {
                    if ('minProp' in meta) {
                        callback(prop + ': ' + 'value must be greater than or equal to ' + meta.minProp); 
                    } else {
                        callback(prop + ': ' + 'value must be greater than or equal to ' + meta.min); 
                    }

                    return;
                }

                if ('max' in meta && newVal > meta.max) {
                    if ('maxProp' in meta) {
                        callback(prop + ': ' + 'value must be less than or equal to ' + meta.maxProp); 
                    } else {
                        callback(prop + ': ' + 'value must be less than or equal to ' + meta.max); 
                    }

                    return;
                }
            }
        }
    
        // At this point the new config is correct. We set the values of the 
        // old config to those of the new config. Don't want to do assignment
        // in case the new config contains extraneous data.
        for (var prop in newConfig) {
            config[prop] = newConfig[prop];
        }

        callback();
    }

    /**
     * Get the meta object for the given configuration property.
     * 
     * @param {string} prop The configuration property.
     * @param {Function} callback Called with the resulting meta object for 
     *                   property or possible error message.
     */
    function getConfigMeta(prop, callback) {
        if (!prop in config) {
            callback(undefined, '\"' + prop + '\" is not a configuration property.');

            return;
        }

        var metaObject = getConfigMetaObject(config);
        callback(metaObject[prop]);
    }

    /**
     * Set the configuration value for a property. Checks the values based
     * on the associated meta data.
     * 
     * @param {string} prop The configuration property.
     * @param {Object} val The value of the configuration property.
     * @param {Function} callback Called with the resulting value and possible
     *                            error message.
     */
    function setConfigVal(prop, val, callback) {
        if (!prop in config) {
            callback(undefined, '\"' + prop + '\" is not a configuration property.');

            return;
        }

        var metaObject = getConfigMetaObject(config);
        var meta = metaObject[prop];
        var newVal;

        // Check the type.
        // Different types have different validations.
        if (meta.type == 'boolean') {   
            newVal = Boolean(val);
        } else if (meta.type == 'string') {
            newVal = String(val);

            if (newVal.length > 128) {
                callback(config[prop], prop + ': field cannot exceed 128 characters'); 

                return;
            }

            var commaIndex = newVal.indexOf(',');

            if (commaIndex > -1) {
                callback(config[prop], prop + ': field cannot contain commas'); 

                return;
            }
        } else if (meta.type == 'csv-string') {
            newVal = String(val);

            if (newVal.length > 128) {
                callback(config[prop], prop + ': field cannot exceed 128 characters'); 

                return;
            } 

        } else if (meta.type == 'integer') {
            newVal = Number(val);

            if (isNaN(newVal)) {
                callback(config[prop], prop + ': value is not a number'); 

                return;
            }

            if (!Number.isInteger(newVal)) {
                callback(config[prop], prop + ': value is not an integer.');

                return;
            }

            if ('min' in meta && newVal < meta.min) {
                if ('minProp' in meta) {
                    callback(config[prop], prop + ': value must be greater than or equal to ' + meta.minProp); 
                } else {
                    callback(config[prop], prop + ': value must be greater than or equal to ' + meta.min); 
                }

                return;
            }

            if ('max' in meta && newVal > meta.max) {
                if ('maxProp' in meta) {
                    callback(config[prop], prop + ': value must be less than or equal to ' + meta.maxProp); 
                } else {
                    callback(config[prop], prop + ': value must be less than or equal to ' + meta.max); 
                }

                return;
            }
        } else if (meta.type == 'float') {
            newVal = Number(val);

            if (isNaN(newVal)) {
                callback(config[prop], prop + ': value is not a number'); 

                return;
            }

            if ('min' in meta && newVal < meta.min) {
                if ('minProp' in meta) {
                    callback(config[prop], prop + ': value must be greater than or equal to ' + meta.minProp); 
                } else {
                    callback(config[prop], prop + ': value must be greater than or equal to ' + meta.min); 
                }

                return;
            }

            if ('max' in meta && newVal > meta.max) {
                if ('maxProp' in meta) {
                    callback(config[prop], prop + ': value must be less than or equal to ' + meta.maxProp); 
                } else {
                    callback(config[prop], prop + ': value must be less than or equal to ' + meta.max); 
                }

                return;
            }
        }

        config[prop] = newVal;
        callback(newVal);
    }

    /**
     * Allow access to the configuration.
     *
     * @param {Function} callback Called for each configuration property.
     */
    function getConfig(callback) {
        for (var prop in config) {
            callback(prop);
        }
    }

    /**
     * Allow access to single configuration property.
     * 
     * @param {string} prop The configuration property.
     * @param {Function} callback Called with value associated with property. 
     *                            called with undefined if the property is not
     *                            in configuration.
     */
    function getConfigVal(prop, callback) {
        if (!prop in config) {
            callback(undefined, undefined, 'prop: \"' + prop + '\" is not a configuration property.');
        }

        var configMetaObject = getConfigMetaObject(config);
        callback(config[prop], configMetaObject[prop]);
    }

    return {
        runStateCallback: runStateCallback,
        runDI: runDI,
        addInputFile: addInputFile,
        removeInputFile: removeInputFile,
        getInputFiles: getInputFiles,
        setOutputDir: setOutputDir,
        setQuadFile: setQuadFile,
        setLogFile: setLogFile,
        getOutputDir: getOutputDir,
        getQuadFile: getQuadFile,
        getLogFile: getLogFile,
        setConfigVal: setConfigVal,
        setNewConfig: setNewConfig,
        getConfig: getConfig,
        getConfigVal: getConfigVal,
        getConfigMeta: getConfigMeta
    }
} ());
