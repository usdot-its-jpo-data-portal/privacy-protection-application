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
 * Defines the graphical user interface. This module handles all non-tab 
 * graphical objects.
 * 
 * This object is responsible for the log, 
 */
var GUI = (function () {
    var TABS = {};
    var activeTab = null;
    var tabSwitchInPorgress = false; 
    var tabElems = null;
    var tabElem = null;

    function init() {
        var operatingSystem;

        // check which operating system is user running
        if (navigator.appVersion.indexOf("Win") != -1) {
            operatingSystem = "Windows";
        } else if (navigator.appVersion.indexOf("Mac") != -1) {
            operatingSystem = "MacOS";
        } else if (navigator.appVersion.indexOf("CrOS") != -1) {
            operatingSystem = "ChromeOS";
        } else if (navigator.appVersion.indexOf("Linux") != -1) {
            operatingSystem = "Linux";
        } else if (navigator.appVersion.indexOf("X11") != -1) {
            operatingSystem = "UNIX";
        } else {
            operatingSystem = "Unknown";
        }

        // Log the operating system.
        GUI.log(operatingSystem);

        // Save the list of tab elements.
        tabElems = $('#tabs > ul');

        // For each tab, create a hook that handles the changing of tabs.
        $('a', tabElems).click(function (e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            // Save the clicked tab. This allows it to be referenced at the 
            // end of the tab swtich callback chain.
            tabElem = $(this);
           
            // Guard against switching from active tab to active tab.
            // Also guard against switching while a switch is in progress.
            if ($(this).parent().hasClass('active') == false && !GUI.tabSwitchInProgress) { 

                if (GUI.activeTab) {
                    TABS[GUI.activeTab].cleanup(tabSwitchCallback);
                } else {
                    tabSwitchCallback();
                }
            }
        });

        // Force click the first tab.
        $('#tabs ul.tab-list li a:first').click();

        $("#showlog").on('click', function(e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            var state = $(this).data('state');

            if ( state ) {
                $("#log").animate({height: 27}, 200, function() {
                     var command_log = $('div#log');
                     command_log.scrollTop($('div.wrapper', command_log).height());
                });
                $("#log").removeClass('active');
                $("#content").removeClass('logopen');
                $(".tab_container").removeClass('logopen');
                $("#scrollicon").removeClass('active');
            
                state = false;
            } else {
                $("#log").animate({height: 111}, 200);
                $("#log").addClass('active');
                $("#content").addClass('logopen');
                $(".tab_container").addClass('logopen');
                $("#scrollicon").addClass('active');

                state = true;
            }

            $(this).text(state ? 'Hide Log' : 'Show Log');
            $(this).data('state', state);
        });
    
        // Set the callback of the DI module so we can enable/disable the
        // the run button.
        DIModule.runStateCallback = function (isRunnable) {
            if (isRunnable) {
                enableRunButton();
            } else {
                disabledRunButton();
            }
        };

        $(".run_btn").on('click', function(e) {
            // Note that this prevents shift-clicking from trying to open
            // a link in a chrome browser.
            e.preventDefault();

            resetProgress();

            DIModule.runDI();
        });

        // The run button is initially disabled.
        disabledRunButton();
    }

    /**
     * Display a timestamped message in to GUI log. The message levels are:
     * 0 or undefined == normal, 1 == warning, >= 2 == error.
     *
     * @param {string} message The message to log.
     * @param {number} level The log level.
     */
    function log(message, level) {
        var commandLog = $('div#log');
        var d = new Date();
        var year = d.getFullYear();
        var month = ((d.getMonth() < 9) ? '0' + (d.getMonth() + 1) : (d.getMonth() + 1));
        var date =  ((d.getDate() < 10) ? '0' + d.getDate() : d.getDate());
        var time = ((d.getHours() < 10) ? '0' + d.getHours(): d.getHours())
             + ':' + ((d.getMinutes() < 10) ? '0' + d.getMinutes(): d.getMinutes())
             + ':' + ((d.getSeconds() < 10) ? '0' + d.getSeconds(): d.getSeconds());

        var formattedDate = "{0}-{1}-{2} {3}".format(
                                    year,
                                    month,
                                    date,
                                    ' @ ' + time
                                );

        if (level == undefined || level == 0) {
            $('div.wrapper', commandLog).append('<p>' + formattedDate + ' -- ' + message + '</p>');
        } else if (level == 1)  {
            $('div.wrapper', commandLog).append('<p><font color="yellow">' + formattedDate + ' -- ' + '[Warning] ' + message + '</font></p>');
        } else if (level >= 2)  {
            $('div.wrapper', commandLog).append('<p><font color="red">' + formattedDate + ' -- ' + '[Error] ' + message + '</font></p>');
        }
        
        commandLog.scrollTop($('div.wrapper', commandLog).height());
    }

    /** 
     * Callback used when to indicate that tab is loaded and ready.
     */
    function tabContentReady() {
        GUI.tabSwitchInPorgress = false;
    }

    /**
     * Callback used when a tab is switched. Determines which tab was clicked
     * and handles the swtich by initializing the tab. Firhter swtiching is 
     * disabled until the switch is complete.
     */
    function tabSwitchCallback() {
        // Disable previously active tab.
        $('li', tabElems).removeClass('active');

        // Highlight selected tab
        var tabName = tabElem.parent().prop('class').substring(4);
        tabElem.parent().addClass('active');

        // Detach listeners and remove element data
        var content = $('#content');
        content.empty();

        // Display the loading screen.
        $('#cache .data-loading').clone().appendTo(content);

        // Load the tab.
        // TODO Definately a better way to do this.
        switch (tabName) {
            case 'welcome':
                TABS.welcome.initialize(tabContentReady);
                break;
            case 'files':
                TABS.files.initialize(tabContentReady);
                break;
            case 'map':
                TABS.map.initialize(tabContentReady);
                break;
            case 'osm':
                TABS.osm.initialize(tabContentReady);
                break;
            case 'config':
                TABS.config.initialize(tabContentReady);
                break;
            case 'adv_config':
                TABS.adv_config.initialize(tabContentReady);
                break;
            default:
                console.log('Tab not found: ' + tabName);
                break;
        }
    }
    
    /**
     * Initialize the switchery and tool tips for all loaded content. This is
     * called during the initialization routine for each tab as needed.
     * 
     * @param {Function} callback Callback to handle the end of the 
     *                            initialization.
     */
    function contentReady(callback) {
        // Load the small switches.
        $('.togglesmall').each(function(index, elem) {
            var switchery = new Switchery(elem, {
              size: 'small',
              color: '#ff9933',
              secondaryColor: '#c4c4c4'
            });
            $(elem).on('change', function (evt) {
                switchery.setPosition();
            });
            $(elem).removeClass('togglesmall');
        });

        // Load the normal switches.
        $('.toggle').each(function(index, elem) {
            var switchery = new Switchery(elem, {
                color: '#ff9933',
                secondaryColor: '#c4c4c4'
            });
            $(elem).on('change', function (evt) {
                switchery.setPosition();
            });
            $(elem).removeClass('toggle');
        });

        // Load the medium switches.
        $('.togglemedium').each(function(index, elem) {
            var switchery = new Switchery(elem, {
                className: 'switcherymid',
                color: '#ff9933',
                secondaryColor: '#c4c4c4'
             });
             $(elem).on('change', function (evt) {
                 switchery.setPosition();
             });
             $(elem).removeClass('togglemedium');
        });

        // Load the CF tooltips.
        jQuery(document).ready(function($) {
            $('cf_tip').each(function() { // Grab all ".cf_tip" elements, and for each...
                log(this); // ...print out "this", which now refers to each ".cf_tip" DOM element
            });

            $('.cf_tip').each(function() {
                $(this).jBox('Tooltip', {            
                    delayOpen: 100,
                    delayClose: 100,
                    position: {
                        x: 'right',
                        y: 'center'
                    },
                    outside: 'x'
                });
            });
        });
    
        if (callback) callback();
    }

    /**
     * Set the progress to fill the specified percentage of the bar.
     *
     * @param {number} progress The percentage of the bar to occupy. 
     */
    function updateProgress(progress) {
        $('.progress_fill').width(progress.toString() + '%');
    }

    /**
     * Reset the progress to occupy zero percent of the bar.
     */
    function resetProgress() {
        $('.progress_fill').width('0%');
    }

    /**
     * Enable the run button.
     */
    function enableRunButton() {
        $(".run_btn").removeClass("disabled");
    }

    /**
     * Disable the run button. 
     */
    function disabledRunButton() {
        $(".run_btn").addClass("disabled");
    }
    
    return {
        TABS: TABS,
        contentReady: contentReady,
        init: init,
        activeTab: activeTab,
        log: log,
        updateProgress: updateProgress,
        resetProgress: resetProgress,
        enableRunButton: enableRunButton,
        disabledRunButton: disabledRunButton
    };
} ());
