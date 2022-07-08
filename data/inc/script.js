
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
moment_mins=10
wifi_on=5
app_name=test light
ip_last_num=33`;
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
    if (TEST)
        console.log(configs);
    DisplayConfig();
}

function docel(id) { return document.getElementById(id); }

function DisplayConfig() {
    docel('chkAuto').checked = ConfValue('auto') == '1';
    docel('timAutoFrom').value = ConfValue('auto_from');
    docel('timAutoTo').value = ConfValue('auto_to');
    docel('chkMoment').checked = false;
    docel('numMoment').value = ConfValue('moment_mins');
    docel('numWiFiOn').value = ConfValue('wifi_on');
    app_name = ConfValue('app_name');
    docel('h1').innerText = document.title = "ESP Relay - " + app_name;
    ipLastNum = parseInt(ConfValue('ip_last_num'));
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
        + 'moment_mins' + sepProps + docel('numMoment').value + sepParams
        + 'wifi_on' + sepProps + docel('numWiFiOn').value + sepParams
        + 'app_name' + sepProps + app_name + sepParams
        + 'ip_last_num' + sepProps + ipLastNum
        ;

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

var app_name;
var ipLastNum;

function ChangeNameIp() {
    var temp = prompt('App Name', app_name);
    if (temp != null) {
        app_name = temp;
        docel('h1').innerText = document.title = "ESP Relay - " + app_name;
    }
    temp = prompt('Last IP address number', ipLastNum);
    if (temp != null && !isNaN(parseInt(temp)))
        ipLastNum = parseInt(temp);
    SaveConfig();
}

function GetDeviceTime() {
    if (TEST)
        spanDeviceTime.innerText = '12:34:56';
    else {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200)
                spanDeviceTime.innerText = this.responseText;
        };
        xhttp.open('GET', 'getDeviceTime', true); xhttp.send();
    }
}

function GetCurrentTime() {
    var xhttp = new XMLHttpRequest();
    xhttp.open('GET', 'getCurrentTime', true);
    xhttp.send();
}