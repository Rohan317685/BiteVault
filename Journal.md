## Idea evolution + Starting on BiteVault - Day 1 --- 4.2 hours
Today has been a pretty somewhat productive day, I first was doing messing around with the concept of devboards and quickly realised that nowadays people try to maximise everything in a PCB. They add extra features no one asked for and they are **EXPENSIVE**.

I decided to make a series project, where I made a bunch of PCB's that have a certain cool project, are bite sized, cheap and tiny like im talking some smaller than 20 x 20mm! These will use very cheap controller and look VERY cool.

By the end of this I want to create like 5-8 of these and have a very cool lineup of these mini PCB's. 

After I finished planning I decided to start on my first BiteBoard it's called BiteVault it is a FIDO2/U2F Security key, crypto key storage and encrypted usb token. I did some research and found that CH32V203 is the best cheap MCU for this use case, ATECC608B is a good secure element that stores private cryptographic keys and XC6206P332MR is a VERY good power supply regulator and is dirt cheap.

---------------------------------------------------------------------------------------------

After researching all these item's I started making my PCB but I couldn't find a proper CH32V203 symbol so I made my first ever symbol using the datasheet and had to work out what each pin did and if their input, bidirectional, passive etc!

I had to figure out what the hell to pick in this which I did eventually.

![image](https://cdn.hackclub.com/019fba3c-e32d-7529-9b04-cc8352beb637/Screenshot%20from%202026-07-31%2015-25-00.png)

Had to figure out whether each pin was passive, bidirectional etc which took a bit.

![image](https://cdn.hackclub.com/019fba3d-b6a6-7f84-9560-1ed946ec4ac7/Screenshot%20from%202026-07-31%2015-42-42.png)

Finished the symbol here!

![image](https://cdn.hackclub.com/019fba3e-35b8-7c99-854f-5109e3a9fe8c/Screenshot%20from%202026-07-31%2016-10-01.png)

This is the progress I made sofar on the schematic, it's been a pretty productive day overall with a lot of planning and ideation.

![image](https://cdn.hackclub.com/019fba3f-2633-7b91-80d1-777577606e51/Screenshot%20from%202026-07-31%2023-15-19.png)

For the voltage regulator symbol I'm using a different symbol and matching up the pin's so it may not look correct here but it is correct because the pin system variants and VI is VO on the actual voltage regulator I'm using so its flipped.
