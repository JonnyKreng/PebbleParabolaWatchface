"use strict";
Pebble.addEventListener("ready", function (_e) {
    console.log("Weather widget initialized.");
    fetchWeather();
    // Update weather every 15 minutes (900,000 milliseconds)
    setInterval(fetchWeather, 900000);
});
Pebble.addEventListener("appmessage", function (_e) {
    console.log("App message event received");
});
function fetchWeather() {
    if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(function (position) {
            var latitude = position.coords.latitude;
            var longitude = position.coords.longitude;
            console.log("Got location: ".concat(latitude, ", ").concat(longitude));
            fetchWeatherForLocation(latitude, longitude);
        }, function (error) {
            console.error("Error getting geolocation:", error.message);
            updateWidget("Location unavailable.");
        });
    }
    else {
        console.error("Geolocation not supported");
        updateWidget("Geolocation unavailable.");
    }
}
function fetchWeatherForLocation(latitude, longitude) {
    var weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=".concat(latitude, "&longitude=").concat(longitude, "&hourly=temperature_2m,relative_humidity_2m,precipitation,precipitation_probability,wind_speed_10m&past_days=0&forecast_days=7");
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        if (xhr.status === 200) {
            try {
                var data = JSON.parse(xhr.responseText);
                var hourlyData = data.hourly;
                if (!hourlyData || hourlyData.temperature_2m.length === 0) {
                    console.error("Could not retrieve hourly weather data.");
                    updateWidget("Weather data unavailable.");
                    return;
                }
                var currentTemp = "".concat(Math.round(hourlyData.temperature_2m[0]), " \u00B0C");
                console.log("Current temperature:", currentTemp);
                updateWidget(currentTemp);
            }
            catch (error) {
                console.error("Error parsing weather data:", error);
                updateWidget("Weather service error.");
            }
        }
        else {
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
function updateWidget(message) {
    console.log("Sending weather update to C app:", message);
    // Send the weather message to the C app using Pebble.sendAppMessage
    Pebble.sendAppMessage({
        "weather_temperature": message
    });
}
