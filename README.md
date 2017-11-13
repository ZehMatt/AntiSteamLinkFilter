# AntiSteamLinkFilter
If you feel frustrated about Steam and Links on the Chat taking forever to open then your frustration shall end here and now.

# What does it do?
Steam wants to check the Link before actually opening it and check the URL against their database for malicious links, this can take a while and there is no way to disable it. This little tool blocks Steam from checking all the unknown Links.

# How does it work?
We will feed steam a dll called "steamconsole.dll" which implements the basic startup of Steam. I discovered this while reversing some of the Steam libraries that whenever the -textclient parameter is passed Steam will try to load "steamconsole.dll". We will try to find the link filter by a pattern and patch it at runtime.

# Advantages
This method has been working for a long time so I decided to put it up here. The big advantage is that after Steam updates you are not required to update this library all you have to ever do is launching Steam with "-textclient" parameter and our "steamconsole.dll"

# Building
You can use Visual Studio 2015 Community or above. Open the AntiSteamLinkFilter.sln and select your configuration to build.

# Installing
Copy your "steamconsole.dll" into the Steam directory alongside Steam.exe, after that simply start Steam.exe with the parameter "-textclient" without the quotes. Steam will launch and the link filtering should be gone.
Ex.: steam.exe -textclient

# Will I get VAC banned?
No
