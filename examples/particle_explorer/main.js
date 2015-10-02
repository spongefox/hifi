//
//  main.js

//  Created by James B. Pollack @imgntn on 9/26/2015
//  Copyright 2015 High Fidelity, Inc.

//  Web app side of the App - contains GUI.
//  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global window, alert, EventBridge, dat, convertBinaryToBoolean, listenForSettingsUpdates,createVec3Folder,createQuatFolder,writeVec3ToInterface,writeDataToInterface*/

var Settings = function() {
    this.exportSettings = function() {
        showPreselectedPrompt();
    };
    this.importSettings = function() {
        importSettings();
    }
};


var AUTO_UPDATE = false;
var UPDATE_ALL_FREQUENCY = 100;

var controllers = [];
var colorControllers = [];
var folders = [];
var gui;
var settings = new Settings();
var updateInterval;

var currentInputField;
var storedController;
var keysToIgnore = [
    'importSettings',
    'exportSettings',
    'script',
    'visible',
    'locked',
    'userData',
    'position',
    'dimensions',
    'rotation',
    'id',
    'description',
    'type',
    'created',
    'age',
    'ageAsText',
    'boundingBox',
    'naturalDimensions',
    'naturalPosition',
    'velocity',
    'gravity',
    'acceleration',
    'damping',
    'restitution',
    'friction',
    'density',
    'lifetime',
    'scriptTimestamp',
    'registrationPoint',
    'angularVelocity',
    'angularDamping',
    'ignoreForCollisions',
    'collisionsWillMove',
    'href',
    'actionData',
    'marketplaceID',
    'collisionSoundURL',
    'shapeType',
    'animationSettings',
    'animationFrameIndex',
    'animationIsPlaying',
    'sittingPoints',
    'originalTextures'
];

var individualKeys = [];
var vec3Keys = [];
var quatKeys = [];
var colorKeys = [];

function convertBinaryToBoolean(value) {
    if (value === 0) {
        return false;
    }
    return true;
}

function loadGUI() {

    //whether or not to autoplace
    gui = new dat.GUI({
        autoPlace: false
    });

    //if not autoplacing, put gui in a custom container
    if (gui.autoPlace === false) {
        var customContainer = document.getElementById('my-gui-container');
        customContainer.appendChild(gui.domElement);
        gui.width = 500;
    }

    // gui.remember(settings);

    //get object keys
    var keys = _.keys(settings);

    //for each key...
    _.each(keys, function(key) {

        var shouldIgnore = _.contains(keysToIgnore, key);
        if (shouldIgnore) {
            return;
        }

        var subKeys = _.keys(settings[key]);
        var hasX = _.contains(subKeys, 'x');
        var hasY = _.contains(subKeys, 'y');
        var hasZ = _.contains(subKeys, 'z');
        var hasW = _.contains(subKeys, 'w');
        var hasRed = _.contains(subKeys, 'red');
        var hasGreen = _.contains(subKeys, 'green');
        var hasBlue = _.contains(subKeys, 'blue');
        if ((hasX && hasY && hasZ) && hasW === false) {
            // console.log(key + " is a vec3");
            vec3Keys.push(key);
        } else if (hasX && hasY && hasZ && hasW) {
            // console.log(key + " is a quaternion");
            quatKeys.push(key);
        } else if (hasRed || hasGreen || hasBlue) {
            // console.log(key + " is a color");
            colorKeys.push(key);

        } else {
            // console.log(key + ' is a single key not an obj')
            individualKeys.push(key);

        }


    });

    individualKeys.sort();
    vec3Keys.sort();
    quatKeys.sort();
    colorKeys.sort();
    gui.add(settings, 'importSettings');
    gui.add(settings, 'exportSettings');
    addIndividualKeys();
    addFolders();
    if (AUTO_UPDATE) {
        setInterval(manuallyUpdateDisplay, UPDATE_ALL_FREQUENCY);
    }
    registerDOMElementsForListenerBlocking();

}

function addIndividualKeys() {
    _.each(individualKeys, function(key) {

        var controller = gui.add(settings, key).listen();
        //need to fix not being able to input values if constantly listening
        //.listen();

        //  keep track of our controller
        controllers.push(controller);

        //hook into change events for this gui controller
        controller.onChange(function(value) {
            // Fires on every change, drag, keypress, etc.
            writeDataToInterface(this.property, value);
        });
    });
}

function addFolders() {
    _.each(colorKeys, function(key) {
        // createColorFolder(key);
        createColorPicker(key);
    });
    _.each(vec3Keys, function(key) {
        createVec3Folder(key);
    });
    _.each(quatKeys, function(key) {
        createQuatFolder(key);
    });

}

function createColorPicker(key) {
    var colorObject = settings[key];
    var colorArray = convertColorObjectToArray(colorObject);
    settings[key] = colorArray;
    var controller = gui.addColor(settings, key);
    controller.onChange(function(value) {
        var obj = {};
        obj[key] = convertColorArrayToObject(value);
        writeVec3ToInterface(obj);
    });
    return;

    // conroller.onChange(function(colorArray) {
    //     var colorObject = convertColorArrayToObject(colorArray);
    //     var obj = {};
    //     obj[key] = colorObject
    //     writeVec3ToInterface(obj)
    //     console.log('color changed, write this to interface' + JSON.stringify(obj))
    // });
    // controllers.push(controller);
}

function createVec3Folder(category) {
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'x').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'y').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        obj[category].z = settings[category].z;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'z').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].y = settings[category].y;
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folders.push(folder);
}

function createQuatFolder(category) {
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'x').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'y').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        obj[category].z = settings[category].z;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'z').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category].y = settings[category].y;
        obj[category][this.property] = value;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'w').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folders.push(folder);
}

function createColorFolder(category) {
    // console.log('CREATING COLOR FOLDER', category)
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'red').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].green = settings[category].green;
        obj[category].blue = settings[category].blue;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'green').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};

        obj[category].red = settings[category].red;
        obj[category][this.property] = value;
        obj[category].blue = settings[category].blue;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'blue').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].red = settings[category].red;
        obj[category].green = settings[category].green;

        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folders.push(folder);
}


function convertColorObjectToArray(colorObject) {
    var colorArray = [];
    _.each(colorObject, function(singleColor) {
        colorArray.push(singleColor);
    })
    return colorArray
}

function convertColorArrayToObject(colorArray) {
    var colorObject = {
        red: colorArray[0],
        green: colorArray[1],
        blue: colorArray[2]
    }
    return colorObject
}

function writeDataToInterface(property, value) {
    var data = {};
    data[property] = value;
    var sendData = {
        messageType: "settings_update",
        updatedSettings: data,
    };

    var stringifiedData = JSON.stringify(sendData);

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

function writeVec3ToInterface(obj) {
    var sendData = {
        messageType: "settings_update",
        updatedSettings: obj,
    };

    var stringifiedData = JSON.stringify(sendData);

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

window.onload = function() {
    console.log('WINDOW ONLOAD');
    if (typeof EventBridge !== 'undefined') {

        var stringifiedData = JSON.stringify({
            messageType: 'page_loaded'
        });

        EventBridge.emitWebEvent(
            stringifiedData
        );

        listenForSettingsUpdates();
    } else {
        console.log('No event bridge, probably not in interface.');
    }

};

function listenForSettingsUpdates() {
    // console.log('GUI IS LISTENING FOR MESSAGES FROM INTERFACE');
    EventBridge.scriptEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.messageType === 'object_update') {
            _.each(data.objectSettings, function(value, key) {
                //  settings[key] = value;
            });
        }

        if (data.messageType === 'initial_settings') {
            // console.log('INITIAL SETTINGS FROM INTERFACE:::' + JSON.stringify(data.initialSettings));
            _.each(data.initialSettings, function(value, key) {
                settings[key] = {};
                settings[key] = value;
            });

            loadGUI();
        }

        // if (data.messageType === 'settings_update') {
        //     console.log('SETTINGS UPDATE FROM INTERFACE:::' + JSON.stringify(data.updatedSettings));
        //     _.each(data.updatedSettings, function(value, key) {
        //         settings[key] = value;
        //     });
        // }

    });

}


function manuallyUpdateDisplay() {
    // Iterate over all controllers
    // this is expensive, write a method for indiviudal controllers and use it when the value is different than a cached value, perhaps.
    var i;
    for (i in gui.__controllers) {
        gui.__controllers[i].updateDisplay();
    }
}

function removeContainerDomElement() {
    var elem = document.getElementById("my-gui-container");
    elem.parentNode.removeChild(elem);
}

function showParticleSettings() {
    var codeBlock = document.getElementById("export-code");
    codeBlock.innerHTML = prepareSettingsForExport();
}

function prepareSettingsForExport() {
    var keys = _.keys(settings);
    var exportSettings = {};
    //for each key...
    _.each(keys, function(key) {
        var shouldIgnore = _.contains(keysToIgnore, key);
        if (shouldIgnore) {
            return;
        }

        if (key.indexOf('color') > -1) {
            var colorObject = convertColorArrayToObject(settings[key]);
            settings[key] = colorObject
        }

        exportSettings[key] = settings[key];
    })
    return JSON.stringify(exportSettings);
}


function removeListenerFromGUI(key) {
    console.log('REMOVE ' + key )
    _.each(gui.__listening, function(controller, index) {
        console.log('CONTROLLER AT REMOVE' + controller)
        // if (controller.property === key) {
        //     storedController = controller;
        //     gui.__listening.splice(index, 1);
        // }
    });
}

function addListenersBackToGUI(event) {
    gui.__listening.push(storedController);
    storedController = null;
}

function registerDOMElementsForListenerBlocking() {
    console.log('gui.__controllers length::: '+gui.__controllers.length)

  

    _.each(gui.__controllers, function(controller) {
        var input = controller.domElement.childNodes[0];
        input.addEventListener('focus', function(event) {
            console.log('INPUT ELEMENT GOT FOCUS!' + controller.property);
            removeListenerFromGUI(controller.property);
        });
    })

        _.each(gui.__controllers, function(controller) {
        var input = controller.domElement.childNodes[0];
        input.addEventListener('blur', function(event) {
            console.log('INPUT ELEMENT GOT BLUR!' + controller.property);
            addListenersBackToGUI();
        });
    })

    // _.each(gui.__folders, function(folder) {
    //     _.each(folder.__controllers, function(controller) {
    //         var input = controller.__input;
    //         input.addEventListener('focus', function(event) {
    //             console.log('FOLDER ELEMENT GOT FOCUS!' + controller.property);
    //         });
    //     })
    // });
}

// gui.__folders['Flow Field'].__controllers[0].__input

function importSettings() {
    var importInput = document.getElementById('importer-input');
    console.log('import value' + importInput.value)
    try {
        var importedSettings = JSON.parse(importInput.value);
        // importedSettings = importInput.value;
        var keys = _.keys(importedSettings);
        _.each(keys, function(key) {
            var shouldIgnore = _.contains(keysToIgnore, key);
            if (shouldIgnore) {
                return;
            }
            settings[key] = importedSettings[key];
        });
        writeVec3ToInterface(settings);
        manuallyUpdateDisplay();
    } catch (e) {
        alert('Not properly formatted JSON'); //error in the above string(in this case,yes)!
    }

}

function handleInputKeyPress(e) {
    if (e.keyCode === 13) {
        importSettings();
    }
    return false;
}

function showPreselectedPrompt() {
    window.prompt("Copy to clipboard: Ctrl+C, Enter", prepareSettingsForExport());
}