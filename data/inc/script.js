
const TEST = false;

const sepRows = '\n';
const sepProps = '=';
const sepTime = ';';

class Config {
    constructor(name, value) {
        this.name = name;
        this.value = value;
    }

    toString() {
        return this.name + '=' + this.value;
    }
}

var configs = [];

function GetConfig() {
    if (TEST) {
        const resp =
            `auto=1
auto_from=07:00
auto_to=07:30
moment=0
moment_from=01:08
moment_mins=10`;
        ParseConfig(resp);
    }
    else {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200)
                ParseConfig(this.responseText);
        };
        xhttp.open('GET', 'dat/config.ini', true); xhttp.send();
    }
}

function ParseConfig(resp) {
    var a = resp.split(sepRows);
    for (var s of a) {
        s = s.trim();
        if (s.length === 0) continue;
        const props = s.split(sepProps);
        configs.push(new Config(props[0], props[1]));
    }
    DisplayConfig();
}

function docel(id) { return document.getElementById(id); }

function DisplayConfig() {
    docel('chkAuto').checked = ConfValue('auto') == '1';
    docel('timAutoFrom').value = ConfValue('auto_from');
    docel('timAutoTo').value = ConfValue('auto_to');
    docel('chkMoment').checked = false; //B ConfValue('moment') == '1';
    docel('numMoment').value = ConfValue('moment_mins');
}

function ConfValue(confName) {
    for (const c of configs)
        if (c.name == confName)
            return c.value;
    return '';
}

function SaveConfig() {
    const sepParams = '&';
    const now = new Date();
    const time = now.getHours() + ":" + now.getMinutes();
    const confData
        = 'auto' + sepProps + (docel('chkAuto').checked ? 1 : 0) + sepParams
        + 'auto_from' + sepProps + docel('timAutoFrom').value + sepParams
        + 'auto_to' + sepProps + docel('timAutoTo').value + sepParams
        + 'moment' + sepProps + (docel('chkMoment').checked ? 1 : 0) + sepParams
        + 'moment_from' + sepProps + time + sepParams
        + 'moment_mins' + sepProps + docel('numMoment').value;

    if (TEST)
        console.log(confData);
    else {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200)
                alert("Your data has been saved.");
        };
        xhttp.open('GET', 'save_config?' + confData, true); xhttp.send();
    }
}