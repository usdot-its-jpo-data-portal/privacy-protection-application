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
 * The configuration tab allows the user to update the configuration. The
 * configuration is maintained by the DI module. It is read when the tab 
 * contents are loaded. This tab is also allows the user to select the 
 * required quad file and output directory.
 */
GUI.TABS.config = (function() {
    /**
     * The remote dialog for opening files a directory.
     */
    const {dialog} = require('electron').remote

    /**
     * Initialize the tab. Load the HTML contents and any saved state.
     * 
     * @param {Function} callback The callback to call when the content is 
     *                            loaded.
     */
    function initialize(callback) {
        // All initialize functions must change the active tab.
        if (GUI.activeTab != 'config') {
            GUI.activeTab = 'config';
        }

        // Load the HTML content.
        $('#content').load("./tabs/config.html", function () {
            // Add the localized text to the elements.
            localize();

            // Inform the GUI that the content is reeady.
            // This loads tool tips and transforms the Switchery elements for
            // for the tab.
            GUI.contentReady(callback);

            // Fill default values.
            SharedConfig.updateConfig();
    
            // Get the output directory from the DI module.
            DIModule.getOutputDir(function (outputDir) {
                if (outputDir == undefined) {
                    $('div.out_dir').text('None');
                } else {
                    $('div.out_dir').text(outputDir);
                }
            });

            // Get the quad file from the DI module.
            DIModule.getQuadFile(function (quadFile) {
                if (quadFile == undefined) {
                    $('div.quad_file').text('None');
                } else {
                    $('div.quad_file').text(quadFile);
                }
            });

            // Get the quad file from the DI module.
            DIModule.getLogFile(function (logFile) {
                if (logFile == undefined) {
                    $('div.log_file').text('None');
                } else {
                    $('div.log_file').text(logFile);
                }
            });

            // Hook the output directory button.
            $('a.out_dir_btn').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                var outDir_;

                // Load in the quad file from the config.
                DIModule.getOutputDir(function (outDir) {
                    outDir_ = outDir;
                });

                // Open the output directory.
                dialog.showOpenDialog({properties: ['openDirectory'], defaultPath: outDir_}, function (paths) {
                    // Check for cancellation.
                    if (paths == undefined) {
                        $('div.out_dir').text(outDir_);

                        return;
                    }
    
                    var path = paths[0];

                    DIModule.setOutputDir(path);
                    $('div.out_dir').text(path);
                });
            });

            // Hook the quad file button.
            $('a.quad_file_btn').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                var quadFile_;

                // Load in the quad file from the config.
                DIModule.getQuadFile(function (quadFile) {
                    quadFile_ = quadFile;
                });

                // Open the quad file.
                dialog.showOpenDialog({properties: ['openFile'], defaultPath: quadFile_}, function (paths) {
                    // Check for cancellation.
                    if (paths == undefined) {
                        $('div.quad_file').text(quadFile_);

                        return;
                    }

                    var path = paths[0];

                    DIModule.setQuadFile(path);
                    $('div.quad_file').text(path);
                });
            });

            // Hook the log file button.
            $('a.log_file_btn').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                var logFile_;

                // Load in the log file from the config.
                DIModule.getLogFile(function (logFile) {
                    logFile_ = logFile;
                });

                // Open the log file.
                dialog.showSaveDialog({defaultPath: logFile_}, function (filename) {
                    // Check for cancellation.
                    if (filename == undefined) {
                        $('div.log_file').text(logFile_);

                        return;
                    }

                    var path = filename;

                    DIModule.setLogFile(path);
                    $('div.log_file').text(path);
                });
            });

            SharedConfig.initElements();
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
