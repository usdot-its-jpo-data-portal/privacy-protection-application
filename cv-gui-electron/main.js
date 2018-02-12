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

$(document).ready(function () {
    // Translate to user-selected language.
    localize();

    // Initialize the GUI.
    GUI.init();
});

/**
 * String formatting now supports currying (partial application).
 * For a format string with N replacement indices, you can call .format
 * with M <= N arguments. The result is going to be a format string
 * with N-M replacement indices, properly counting from 0 .. N-M.
 * The following Example should explain the usage of partial applied format:
 * "{0}:{1}:{2}".format("a","b","c") === "{0}:{1}:{2}".format("a","b").
 * format("c")
 * "{0}:{1}:{2}".format("a").format("b").format("c") === "{0}:{1}:{2}".
 * format("a").format("b", "c")
 */
String.prototype.format = function () {
    var args = arguments;

    return this.replace(/\{(\d+)\}/g, function (t, i) {
        return args[i] !== void 0 ? args[i] : "{"+(i-args.length)+"}";
    });
};

/**
 * Get the number of decimal places of a number.
 */
var countDecimals = function (value) {
    if (Math.floor(value) === value) return 0;

    return value.toString().split(".")[1].length || 0; 
};
