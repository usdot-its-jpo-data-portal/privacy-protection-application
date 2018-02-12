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
 * Module responsible for taking all localization elements and translating them 
 * using the localization file. Handles all the i18n* GUI elements. 
 */
function localize() {
    /**
     * Module to get the path of the localization file.
     */
    const path = require("path");

    /**
     * Module to read the localization file.
     */
    const fs = require('fs');
    /**
     * Count of localizations. The is the number of translated messages. 
     */
    var localized = 0;

    var loadedLanguage = JSON.parse(fs.readFileSync(path.join(__dirname,'_locales/en/messages.json')));

    /**
     * Translate a message using a messsage ID.
     * 
     * @param {string} messageID The message ID string to translate.
     */
    function translate(messageID) {
        localized++;
    
        //return chrome.i18n.getMessage(messageID);
        return loadedLanguage[messageID]["message"];
    };

    // For each i18n not aleady replaced, replace the element's message with
    // the translation.
    $('[i18n]:not(.i18n-replaced').each(function() {
        var element = $(this);

        element.html(translate(element.attr('i18n')));
        element.addClass('i18n-replaced');
    });

    // For each i18n title not aleady replaced, replace the element's message 
    // with the translation.
    $('[i18n_title]:not(.i18n_title-replaced').each(function() {
        var element = $(this);

        element.attr('title', translate(element.attr('i18n_title')));
        element.addClass('i18n_title-replaced');
    });

    return localized;
}
