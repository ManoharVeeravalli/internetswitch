#include "IsValid.h"
#include <ArduinoJson.h>

const char STYLES[] PROGMEM = R"=====(
:root {
    --primary: rgb(79 70 229 / 1);
    --primary-hover: rgb(99 102 241);
    --secondary: #fff;
    --background: #f3f3f3;
    --text-color-1: rgb(55 65 81 / 1);
    --text-color-2: rgb(17 24 49 / 1);
    --white-overlay: rgba(255, 255, 255, .5);
    --error: red;
    --color-gray: #b5bdc4;
    font-family: Inter, system-ui, Avenir, Helvetica, Arial, sans-serif;
    line-height: 1.5;
    font-synthesis: none;
}

.flex-center {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}

body {
    background-color: var(--background);
    box-sizing: border-box;

}
.index-box {
    width: 80%;
}

.box {
    border: 1px solid var(--color-gray);
    background-color: var(--secondary);
    margin: 20px;
    border-radius: 5px;
    padding: 30px;
}

header {
    padding: 0px 10px;
    display: flex;
    flex-direction: row;
    justify-content: space-between;
}

.heading {
    text-align: center;
    color: var(--text-color-2)
}

button {
    padding: 10px 20px;
    text-align: center;
    display: flex;
    justify-content: center;
    align-items: center;
    color: var(--secondary);
    background-color: var(--primary);
    border-radius: .375rem;
    border: 0;
    cursor: pointer;
}

button:hover,
button:active {
    background-color: var(--primary-hover);
}

button:disabled,
button[disabled] {
    border: 1px solid var(--secondary);
    background-color: var(--background);
    color: var(--primary);
}

button:disabled,
button[disabled]:hover,
button[disabled]:active {
    background-color: var(--background);
}


ul {
    list-style: none;
    padding: 10px;
}

li {
    padding: 10px;
    cursor: pointer;
    color: var(--text-color-1);
    font-weight: 400;
    font-size: 1rem;
    border-radius: 5px;
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    align-items: center;
}

li:hover {
    color: var(--text-color-2);
    background-color: var(--background);
}

li>div {
    display: flex;
    flex-direction: row;
    padding: 5px;
    align-items: center;
}

.loader {
    border: 5px solid var(--background);
    -webkit-animation: spin 1s linear infinite;
    animation: spin 1s linear infinite;
    border-top: 5px solid var(--primary);
    border-radius: 50%;
    width: 30px;
    height: 30px;
    z-index: 3;
    position: relative;
    top: 20%;
    left: 50%;
}

.signal-icon {
    height: 18px;
    width: 18px;
    padding-right: 10px;
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    align-items: baseline;
}

.signal-icon .signal-bar {
    width: 4px;
    opacity: 30%;
    background: var(--primary);
}

.signal-icon .signal-bar:nth-child(1) {
    height: 40%;
}

.signal-icon .signal-bar:nth-child(2) {
    height: 70%;
}

.signal-icon .signal-bar:nth-child(3) {
    height: 100%;
}

.signal-icon.weak .signal-bar:nth-child(1),
.signal-icon.medium .signal-bar:nth-child(1),
.signal-icon.medium .signal-bar:nth-child(2),
.signal-icon.strong .signal-bar:nth-child(1),
.signal-icon.strong .signal-bar:nth-child(2),
.signal-icon.strong .signal-bar:nth-child(3) {
    opacity: 100%;
}

@keyframes spin {
    0% {
        transform: rotate(0deg);
    }

    100% {
        transform: rotate(360deg);
    }
}

.white-overlay {
    position: absolute;
    content: '';
    display: block;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 2;
    background: var(--white-overlay);
}

.modal {
    display: none;
    position: fixed;
    z-index: 1;
    padding-top: 100px;
    left: 0;
    top: 0;
    width: 100%;
    height: 100%;
    overflow: auto;
    background-color: rgb(0, 0, 0);
    background-color: rgba(0, 0, 0, 0.4);
}

.modal-content {
    position: relative;
    background-color: var(--secondary);
    margin: auto;
    padding: 10px;
    border: 1px solid var(--primary-hover);
    width: 40%;
    border-radius: 10px;
    box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19);
    -webkit-animation-name: animatetop;
    -webkit-animation-duration: 0.4s;
    animation-name: animatetop;
    animation-duration: 0.4s
}

@-webkit-keyframes animatetop {
    from {
        top: -300px;
        opacity: 0
    }

    to {
        top: 0;
        opacity: 1
    }
}

@keyframes animatetop {
    from {
        top: -300px;
        opacity: 0
    }

    to {
        top: 0;
        opacity: 1
    }
}

.close {
    color: var(--primary);
    ;
    float: right;
    font-size: 28px;
    font-weight: bold;
}

.close:hover,
.close:focus {
    text-decoration: none;
    cursor: pointer;
}

.modal-header {
    display: flex;
    flex-direction: row;
    justify-content: space-between;
}

.modal-heading {
    display: flex;
    justify-content: center;
    align-items: center;
}

.modal-heading>* {
    padding: 5px;
}

.modal-body {
    margin: 10px 10px;
}

input {
    width: 100%;
    border: none;
    border-bottom: 1px solid var(--primary-hover);
    outline: none;
    padding: 10px 0px;
    margin-bottom: 25px;
}


.password-input-error {
    border-bottom: 1px solid var(--error) !important;
}

.password-input:focus {
    border-bottom: 1px solid var(--primary);
}

.password-error {
    display: none;
    color: var(--error);
    font-size: 12px;
    position: absolute;
}

.password-success {
    display: none;
    color: green;
    font-size: 12px;
    position: absolute;
}

.error-box,
.success-box {
    height: 10px;
    position: relative;
}

.modal-footer {
    margin-top: 20px;
    display: flex;
    justify-content: flex-end;
}

@media screen and (max-width: 1024px) {
    .modal-content {
        width: 80%;
    }
    .index-box {
        width: 95%;
    }
    .box {
        padding: 10px;
    }
}


.button-loader {
    border: 2px solid var(--background);
    -webkit-animation: spin 1s linear infinite;
    animation: spin 1s linear infinite;
    border-top: 2px solid var(--primary);
    border-radius: 50%;
    width: 10px;
    height: 10px;
    margin-right: 10px;
}

.form-box {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    height: 450px; 
}
.form-body {
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: space-evenly;
}
.form-heading{
    margin-top: 50px;
}
)=====";

const char SCRIPT[] PROGMEM = R"=====(
const LOCK_SVG = `
        <?xml version='1.0' encoding='utf-8'?><svg version='1.1' id='Layer_1'  x='0px' y='0px' width='18px' height='18px'
            viewBox='0 0 96.108 122.88' enable-background='new 0 0 96.108 122.88' xml:space='preserve'>
            <g>
                <path fill-rule='evenodd' clip-rule='evenodd'
                    d='M2.892,56.036h8.959v-1.075V37.117c0-10.205,4.177-19.484,10.898-26.207v-0.009 C29.473,
                    4.177,38.754,0,48.966,0C59.17,0,68.449,4.177,75.173,10.901l0.01,0.009c6.721,6.723,10.898,16.002,10.898,26.207v17.844 v1.075h7.136c1.59,0
                    ,2.892,1.302,2.892,2.891v61.062c0,1.589-1.302,2.891-2.892,2.891H2.892c-1.59,0-2.892-1.302-2.892-2.891 V58.927C0,
                    57.338,1.302,56.036,2.892,56.036L2.892,
                    56.036z M26.271,56.036h45.387v-1.075V36.911c0-6.24-2.554-11.917-6.662-16.03 l-0.005,0.004c-4.111-4.114-9.787-6.669-16.025-6.669c-6.241,
                    0-11.917,2.554-16.033,6.665c-4.109,4.113-6.662,9.79-6.662,16.03 v18.051V56.036L26.271,56.036z M49.149,89.448l4.581,21.139l-12.557,
                    0.053l3.685-21.423c-3.431-1.1-5.918-4.315-5.918-8.111 c0-4.701,3.81-8.511,8.513-8.511c4.698,0,8.511,3.81,8.511,8.511C55.964,85.226,
                    53.036,88.663,49.149,89.448L49.149,89.448z' />
            </g>
        </svg>
        `;
const ENC_NONE = 7;

function onSsidClick(ssid, rssi) {
    document.querySelector('#modal-heading').innerHTML = `
            ${getSignalStrengthHtml(+rssi)}
                <h3>${ssid}</h3>
            `;
    document.querySelector('#myModal').style.display = 'block';
    document.querySelector('#submit-button').setAttribute('ssid', ssid);
    document.getElementById('password').focus();
}
function closeModal() {
    document.querySelector('#myModal').style.display = 'none';
    document.getElementById('password').value = '';
    document.querySelector('#submit-button').removeAttribute('ssid');
    document.getElementById('password').classList.remove('password-input-error');
    document.getElementById('password-error').style.display = 'none';
    document.getElementById('password-error').innerText = '';
    document.getElementById('button-loader').style.display = 'none';
    document.getElementById('password-success').style.display = 'none';
    document.getElementById('password-success').innerText = '';
}
function onInputChange() {
    document.getElementById('password-error').style.display = 'none';
    document.getElementById('password').classList.remove('password-input-error');
    document.getElementById('password-success').style.display = 'none';
}
async function onPasswordSubmit(e) {

    const ssid = e.getAttribute('ssid');
    const password = document.getElementById('password').value;
    if (!password) {
        document.getElementById('password-error').innerText = 'Please enter password';
        document.getElementById('password').classList.add('password-input-error');
        document.getElementById('password-error').style.display = 'block';
        document.getElementById('password').focus();
        return;
    }
    try {
        document.getElementById('button-loader').style.display = 'block';
        document.getElementById('submit-button').setAttribute("disabled", "true");
        let response = await fetch('/save', {
            method: 'POST',
            body: JSON.stringify({ ssid, password }),
            headers: {
                "Content-Type": "application/json",
            },
        })
        let text = await response.text();
        if (response.status !== 200) {
            document.getElementById('password-error').style.display = 'block';
            document.getElementById('password-error').innerText = text || 'Some error has occured, please try again later!';
            document.getElementById('password').focus();
            return;
        }
        document.getElementById('password-success').style.display = 'block';
        document.getElementById('password-success').innerText = text;
        window.location.href = '/';
    } catch (e) {
        console.error(e);
        document.getElementById('password-error').style.display = 'block';
        document.getElementById('password-error').innerText = 'Some error has occured, please try again later!';
    } finally {
        document.getElementById('submit-button').removeAttribute("disabled");
        document.getElementById('button-loader').style.display = 'none';
    }
}
async function refresh(refreshButton) {
    document.getElementById('refresh-error').style.display = 'none';
    document.querySelector('#white-overlay').style.display = 'block';
    try {
        if (refreshButton) {
            document.getElementById('refresh-loader').style.display = 'block';
            document.getElementById('refresh-button').setAttribute("disabled", "true");
        }
        const response = await fetch('/scan');
        const wifis = await response.json();
        document.querySelector('#wifis').innerHTML = wifis.map(wifi => `
                <li id='${wifi.ssid}' onclick='onSsidClick(\"${wifi.ssid}\", \"${wifi.rssi}\")'>
                    <div>
                        <div>
                        ${getSignalStrengthHtml(wifi.rssi)}
                        </div>
                        <div>
                        ${wifi.ssid}
                        </div>
                    </div>
                    <div>
                        ${wifi.encryptionType === ENC_NONE ? '' : LOCK_SVG}
                    </div>
                </li>`)
            .join('');
    } catch (e) {
        document.getElementById('refresh-error').style.display = 'block';
    } finally {
        document.querySelector('#white-overlay').style.display = 'none'
        if (refreshButton) {
            document.getElementById('refresh-loader').style.display = 'none';
            document.getElementById('refresh-button').removeAttribute("disabled");
        }
    }
}


function getSignalStrengthHtml(rssi) {
    if (rssi > -30) {
        return `
                <div class='signal-icon strong'>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                </div>
                `;
    }
    if (rssi > -70) {
        return `
                <div class='signal-icon medium'>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                </div>
                `;
    }
    if (rssi > -80) {
        return `
                <div class='signal-icon weak'>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                    <div class='signal-bar'></div>
                </div>
                `;
    }

    return `
            <div class='signal-icon'>
                <div class='signal-bar'></div>
                <div class='signal-bar'></div>
                <div class='signal-bar'></div>
            </div>
            `;
}

function onDocReady() {
    document.getElementById('login-form')?.addEventListener("submit", async function (event) {
        event.preventDefault();
        const email = document.getElementById('form-email').value;
        const password = document.getElementById('form-password').value;

        try {
            document.getElementById('form-loader').style.display = 'block';
            document.getElementById('form-button').setAttribute("disabled", "true");
            let response = await fetch('/login', {
                method: 'POST',
                body: JSON.stringify({ email, password }),
                headers: {
                    "Content-Type": "application/json",
                },
            });
            let text = await response.text();
            if (response.status !== 200) {
                document.getElementById('form-error').style.display = 'block';
                document.getElementById('form-error').innerText = text || 'Some error has occured, please try again later!';
                document.getElementById('form-email').focus();
            } else {
                document.getElementById('register-success').style.innerHTML = text;
                document.getElementById('login-form').style.display = 'none';
                document.getElementById('register-success').style.display = 'block';
            }
        } catch (e) {
            console.error(e);
            document.getElementById('form-error').style.display = 'block';
            document.getElementById('form-error').innerText = 'Some error has occured, please try again later!';
        } finally {
            document.getElementById('form-loader').style.display = 'none';
            document.getElementById('form-button').removeAttribute("disabled");
        }
    });
}

function onFormChange() {
    document.getElementById('form-error').style.display = 'none';
    document.getElementById('form-error').innerText = '';
    document.getElementById('form-success').style.display = 'none';
    document.getElementById('form-success').innerText = '';
}

)=====";



const char WIFI_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>

<head>
    <title>Internet Switch Setup</title>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <script src="/script"></script>
    <link href="/styles" rel="stylesheet" type="text/css" />
</head>

<body class='flex-center' onload='refresh(false); return false'>
    <div class='box index-box'>
        <div id='white-overlay' class='white-overlay'>
        </div>
        <header>
            <div>
                <h2 class='heading'>Internet <span style='color: var(--primary);'>Switch</span></h2>
            </div>
            <div class='flex-center'>
                <button onclick='refresh(true)' id="refresh-button">
                    <div id="refresh-loader" class='button-loader' style="display: none;"></div> Refresh
                </button>
            </div>
        </header>
        <div>
            <div class="flex-center error-box">
                <p id='refresh-error' class='password-error'>Somer error has occured please try again later</p>
            </div>

            <ul id='wifis'></ul>
        </div>
    </div>
    <div id='myModal' class='modal'>
        <div class='modal-content'>
            <div class='modal-header'>
                <div id='modal-heading' class='modal-heading'></div>
                <span class='close flex-center' onclick='closeModal()'>&times;</span>
            </div>
            <div class='modal-body flex-center'>
                <input id='password' onkeyup='onInputChange()' required class='password-input' type='password'
                    placeholder='Please enter password' />
            </div>
            <div class="flex-center error-box">
                <p id='password-error' class='password-error'></p>
            </div>
            <div class="flex-center success-box">
                <p id='password-success' class='password-success'>Saved Successfully</p>
            </div>
            <div class='modal-footer'>
                <button id='submit-button' class='modal-submit-button' type='submit' onclick='onPasswordSubmit(this)'>
                    <div id="button-loader" class='button-loader' style="display: none;"></div> Submit
                </button>
            </div>

        </div>

    </div>
</body>

</html>
)=====";

const char LOGIN_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>

<head>
    <title>Internet Switch Setup</title>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <script src="/script"></script>
    <link href="/styles" rel="stylesheet" type="text/css" />
</head>

<body class="flex-center" style="height: 90vh;" onload="onDocReady()">
    <form id="login-form">
        <div class="box form-box">
            <h2 class='heading form-heading'>Internet <span style='color: var(--primary);'>Switch</span></h2>
            <div class="form-body">

                <div>
                    <div class="flex-center error-box">
                        <p id='form-error' class='password-error'></p>
                    </div>
                    <div class="flex-center success-box">
                        <p id='form-success' class='password-success'>Saved Successfully</p>
                    </div>
                    <input type="email" onkeyup='onFormChange()' id="form-email" class='password-input' required
                        placeholder='Email ID' />
                    <input type="password" onkeyup='onFormChange()' id="form-password" class='password-input' required
                        placeholder='Password' />
                </div>

                <button id="form-button" type="submit">
                    <div id="form-loader" class='button-loader' style="display: none;"></div>Submit
                </button>
            </div>

        </div>
    </form>
    <h2 style="display: none;" id="register-success" class='heading form-heading'>Device Registered <span
            style='color: var(--primary);'>Successfully</span></span></h2>

</body>

</html>
)=====";

const char SUCCESS_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>

<head>
    <title>Internet Switch Setup</title>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <script src="/script"></script>
    <link href="/styles" rel="stylesheet" type="text/css" />
</head>

<body class="flex-center" style="height: 90vh;">
      <h2 id="register-success" class='heading form-heading'>Device Registered <span
            style='color: var(--primary);'>Successfully</span></span></h2>
</body>

</html>
)=====";
