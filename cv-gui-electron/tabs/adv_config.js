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
 * The advanced configuration tab allows the user to further update the 
 * configuration. The  configuration is maintained by the DI module. It is read 
 * when the tab contents are loaded. This tab is also the user to control more
 * advanced DI features.
 */
GUI.TABS.adv_config = (function() {
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
        $('#content').load("./tabs/adv_config.html", function () {
            // Add the localized text to the elements.
            localize();

            // Inform the GUI that the content is reeady.
            // This loads tool tips and transforms the Switchery elements for
            // for the tab.
            GUI.contentReady(callback);

            // Fill default values.
            SharedConfig.updateConfig();
            
            // Initialize the shared configuration.
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
