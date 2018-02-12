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
 * The files tab maintains a list of files which can be modified by the 
 * user. It is responsible for updating the DI runner. Its condition for 
 * allowing the de-identifaction routine to run is that there must be at
 * least one file in the list. The tab exports its file list when the 
 * DI routine is run.
 */
GUI.TABS.files = (function() {
    /**
     * The remote dialog for opening files a directory.
     */
    const {dialog} = require('electron').remote

    /**
     * Add a file to the file list. Creates a reference to the file and adds
     * adds hooks for handling clicks.
     * 
     * @param {string} path The path of the input file.
     */
    function addFile(path) {
        // Add the reference.
        $('ul.file_list').append(
            '<li class="file_item"><a href="#" class="file_icon">' + 
            path + '</a></li>'
        );

        // This prevents backslashes from messing with the jquery selector.
        // Should only be an issue with Windows paths.
        var fixedPath = path.replace(/\\/g, '\\\\');

        // Add a hook for the added file icon.
        // The icon is the picture and text.
        // This manages the selection of files by the user.
        $('a.file_icon:contains("' + fixedPath + '")').click(function (e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            if (e.shiftKey) {
                // TODO The user is trying for a multiple selection.
            }

            // Get is the actual list item (file_item).
            var fileItem = $(this).parent();

            // Check if this file is selected.
            if (!fileItem.is('.selected')) {
                // This file is not already selected.
                // Add the selection to the list.
                fileItem.addClass('selected');
            } else { 
                // This file is already selected.
                // Remove the selection from the list. 
                fileItem.removeClass('selected');
            } 

            // Check if at least one file is selected.
            if ($('li.file_item.selected').length > 0) {
                // At lest one file is selected.
                // Enable the remove file button.
                $('a.remove_file').removeClass('disabled');
            } else {
                // No files are selected.
                // Disable the remove file button. 
                $('a.remove_file').addClass('disabled');
            }
        });
    }

    /**
     * Add a chrome file entry selected by the user. This must be a separate
     * function in order to force this block to be exectuted for each
     * fileEntry object passed from the user.
     * 
     * @param {String} filePath Chrome file entry object.
     */
    function addFilePath(filePath) {
        // Check if the file is already in the list.
        if ($('a.file_icon:contains("' + filePath + '")').length) {
            // The file is already in the list. Inform the user a return.
            GUI.log("[Input Files] Not adding duplicate file: " + filePath, 1);

            return;
        }

        // Update the module with the new file.
        DIModule.addInputFile(filePath);

        // Add the file to the list in the GUI.
        addFile(filePath, false);
    }
    
    /**
     * Initialize the tab. Load the HTML contents and any saved state.
     * 
     * @param {Function} callback The callback to call when the content is 
     *                            loaded.
     */
    function initialize(callback) {
        // All initialize functions must change the active tab.
        if (GUI.activeTab != 'files') {
            GUI.activeTab = 'files';
        }

        // Load the HTML content.
        $('#content').load("./tabs/files.html", function () {
            // Add the localized text to the elements.
            localize();

            // Inform the GUI that the content is reeady.
            // This loads tool tips and transforms the Switchery elements for
            // for the tab.
            GUI.contentReady(callback);

            // Initially disable the remove button because no files are 
            // selected when the content is loaded.
            $('a.remove_file').addClass('disabled');
    
            // Add each stored file the file list element.
            DIModule.getInputFiles(function (inputFile) {
                addFile(inputFile);
            });

            // Hook the file list wrapper to allow for deselection of all
            // the files.
            $('div.file_list_wrapper').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();
        
                $('a.remove_file').addClass('disabled');
                $('li.file_item').removeClass('selected');
            }).children().click(function () {
                // Do nothing if a child of this element is clicked.
                return false;
            });

            // Hook the load file button to open a dialog for the user to 
            // select input files.
            $('a.load_file').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                // Tell electron to open a dialog.
                dialog.showOpenDialog({properties: ['openFile', 'multiSelections']}, function (paths) {
                    // Check for an error getting the file.
                    if (paths == undefined) {
                        // There was an error so report it and return.
                        GUI.log("[Input Files] Failed to load input file: User cancelled.", 2);

                        return;
                    }

                    for (var i = 0; i < paths.length; i++) {
                        addFilePath(paths[i]);
                    } 
                });
            });

            // Hook the remove file button to remove selected files from the
            // list.
            $('a.remove_file').click(function (e) {
                // Note that this prevents shift-clicking from trying to open
                // a link in a chrome browser.
                e.preventDefault();

                // Remove each selected item from both the GUI and file map.
                $('li.file_item.selected').each(function () {
                    DIModule.removeInputFile($('li.file_item').index(this));
                    $(this).remove();
                });
    
                // No items are selected, so disable the remove button.
                $(this).addClass('disabled');
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
