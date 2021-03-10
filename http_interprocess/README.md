Example using http for interprocess communcation with a header-only api.

1. Start the server.exe, it acts as the receiving part integrated with uhaptikfabrikenapi.h which 
integrates the received values with those from haptic device and presents to the user application.

It shows the latest received values periodically

2. Now you can either use your web browser and set the get parameters directly: 
   http://localhost:8080/set?btn=1&enc=-123

or build a (bluetooth) application that sets them automatically. client.cpp/exe does that as an example.
More info about http library used here: https://github.com/yhirose/cpp-httplib
