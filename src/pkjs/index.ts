Pebble.addEventListener("ready", function (_e) {
    console.log("Weather widget initialized.");
    fetchWeather();
    // Update weather every 15 minutes (900,000 milliseconds)
    setInterval(fetchWeather, 900000);
});

Pebble.addEventListener("appmessage", function (_e) {
    console.log("App message event received");
});

Pebble.addEventListener("showConfiguration", function () {
    var html =
        '<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Parabola Config</title><style>' +
        "body{font-family:-apple-system,sans-serif;background:#222;color:#fff;padding:20px;margin:0}" +
        "h1{font-size:22px;margin:0 0 20px;text-align:center}" +
        "label{display:block;margin:12px 0 4px;font-size:14px;color:#aaa}" +
        "select{width:100%;padding:10px;font-size:16px;background:#333;color:#fff;border:1px solid #555;border-radius:6px}" +
        "button{width:100%;padding:14px;margin-top:24px;font-size:17px;font-weight:600;background:#007aff;color:#fff;border:none;border-radius:10px}" +
        "</style></head><body>" +
        "<h1>Parabola</h1>" +
        "<label>Foreground Color</label><select id='fg'>" +
        "<option value='0xFFFFFF'>White</option>" +
        "<option value='0x000000'>Black</option>" +
        "<option value='0xFF0000'>Red</option>" +
        "<option value='0x0000FF'>Blue</option>" +
        "<option value='0x00FF00'>Green</option>" +
        "<option value='0xFFFF00'>Yellow</option>" +
        "<option value='0xFF8800'>Orange</option>" +
        "<option value='0x8800FF'>Purple</option>" +
        "<option value='0x00FFFF'>Cyan</option>" +
        "<option value='0xFF00FF'>Magenta</option>" +
        "</select>" +
        "<label>Background Color</label><select id='bg'>" +
        "<option value='0x000000'>Black</option>" +
        "<option value='0xFFFFFF'>White</option>" +
        "<option value='0xFF0000'>Red</option>" +
        "<option value='0x0000FF'>Blue</option>" +
        "<option value='0x00FF00'>Green</option>" +
        "<option value='0xFFFF00'>Yellow</option>" +
        "<option value='0xFF8800'>Orange</option>" +
        "<option value='0x8800FF'>Purple</option>" +
        "<option value='0x00FFFF'>Cyan</option>" +
        "<option value='0xFF00FF'>Magenta</option>" +
        "</select>" +
        "<button onclick='save()'>Save</button>" +
        "<script>function save(){var fg=document.getElementById('fg').value;var bg=document.getElementById('bg').value;document.location='pebblejs://close#'+encodeURIComponent(JSON.stringify({foreground_color:fg,background_color:bg}))}<" + "/script>" +
        "</body></html>";

    var encoded = encodeURIComponent(html);
    Pebble.openURL("data:text/html;charset=utf-8," + encoded);
});

Pebble.addEventListener("webviewclosed", function (e) {
    if (!e || !e.response) return;
    try {
        var config = JSON.parse(decodeURIComponent(e.response));
        if (config.foreground_color) config.foreground_color = parseInt(config.foreground_color);
        if (config.background_color) config.background_color = parseInt(config.background_color);
        console.log("Config received:", JSON.stringify(config));
        Pebble.sendAppMessage(config);
    } catch (err) {
        console.error("Config parse error:", err);
    }
});

function fetchWeather() {
    if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(
            function (position) {
                const latitude = position.coords.latitude;
                const longitude = position.coords.longitude;
                console.log(`Got location: ${latitude}, ${longitude}`);
                fetchWeatherForLocation(latitude, longitude);
            },
            function (error) {
                console.error("Error getting geolocation:", error.message);
                updateWidget("Location unavailable.");
            }
        );
    } else {
        console.error("Geolocation not supported");
        updateWidget("Geolocation unavailable.");
    }
}

function fetchWeatherForLocation(latitude: number, longitude: number) {
    const weatherUrl = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=temperature_2m&hourly=temperature_2m`;

    const xhr = new XMLHttpRequest();

    xhr.onload = function () {
        if (xhr.status === 200) {
            try {
                const data = JSON.parse(xhr.responseText);
                const current = data.current;

                console.log(data);

                if (!current || current.temperature_2m === undefined) {
                    console.error("Could not retrieve current weather data.");
                    updateWidget("Weather data unavailable.");
                    return;
                }

                const currentTemp = `${Math.round(current.temperature_2m)} °C`;
                console.log("Current temperature:", currentTemp);
                updateWidget(currentTemp);
            } catch (error) {
                console.error("Error parsing weather data:", error);
                updateWidget("Weather service error.");
            }
        } else {
            console.error("HTTP error! status:", xhr.status);
            updateWidget("Weather service error.");
        }
    };

    xhr.onerror = function () {
        console.error("Error fetching weather data:", xhr.statusText);
        updateWidget("Weather service error.");
    };

    xhr.open("GET", weatherUrl);
    xhr.send();
}

function updateWidget(message: string) {
    console.log("Sending weather update to C app:", message);
    Pebble.sendAppMessage({
        "weather_temperature": message
    });
}