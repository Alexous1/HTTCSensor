//this function get all the element by her id and assign them in a var

function getValue() {
    var wifi_ssid = document.getElementById("wifi_ssid").value;
    var wifi_password = document.getElementById("wifi_password").value;
    var ip_broker = document.getElementById("ip_broker").value;
    var password_broker = document.getElementById("password_broker").value;
    var user_broker = document.getElementById("user_broker").value;
    var topic_broker = document.getElementById("topic_broker").value;
    var time = document.querySelector('input[type=radio][name=gender]:checked');
    var value = document.getElementById("select");
    var wifi_mode = value.options[value.selectedIndex].text;
}

let id = ["tempValue", "humValue", "co2Value", "tvocValue", "batt"];
let request = ["temperature", "humidity", "co2", "tvoc", "bat"];


// each 10s replace the value of the sensor in the website by the actual value
setInterval(function getData()
{

    for (let nb = 0; nb < 5; nb++){
        // create a request to get the value of each sensor
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function()
        {
            if(this.readyState == 4 && this.status == 200)
            {
                document.getElementById(id[nb]).innerHTML = this.responseText;
            }
        };

        // send the request
        xhttp.open("GET", request[nb], true);
        xhttp.send();

        }

}, 10000);
