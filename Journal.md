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

## Added secure element and misc chores - Day 2 --- 1.4 hours
I've been extremely today however I need that stickersheet so I worked on doing some miscellaneous things like setup the github repository + upload everything, Transfer journals from #macondo which I don't plan to do anymore, research and integrate a secure element, organise the schematic and rename some pins for the funsies. 

---------------------------------------------------------------------------------------------

Organised the schematic, I will further organise it and add a Logo in the middle after everything is done schematic wise 

![image](https://user-cdn.hackclub-assets.com/019ffd1a-1fa1-76b2-9d75-ccf18e69ecab/Screenshot%20from%202026-08-13%2022-48-52.png)

I renamed the PB6 and PB7 to peanut butter 6 and peanut butter 7 cause it reminded me of peanut better 😋

![image](https://cdn.hackclub.com/019ffd1a-20a1-72ed-a596-0cc54b9aa35e/Screenshot%20from%202026-08-13%2022-49-01.png)

Integrated the secure element, I first researched a bunch of options and I found this one to be good quality and very small. 1$ when you buy 1-9 units on LCSC is certainly expensive but it's definitely worth it as its much more secure physically. I also read the documentation for this which you can find here: [CH32V203 Datasheet](https://cdn-learn.adafruit.com/assets/assets/000/131/418/original/CH32V203DS0.PDF?1721655401)

![image](https://user-cdn.hackclub-assets.com/019ffd1a-214a-7576-9234-50f2db4a8145/Screenshot%20from%202026-08-13%2022-49-15.png)

Overall I learned a lot about physical security and cool things about security elements and it was pretty straightforward!

## Added bunch of small features and checking final things - Day 3 --- 2.3 Hours

I've worked on a couple features and looking on finishing the schematic soon™️.

I have added a Status LED it is the APTD1608LSURCK and it seems to be very small and cheap which is perfect! I spent a while researching for a good LED that matches the projects interests, heres the [LCSC page](https://www.lcsc.com/product-detail/C5342734.html). 

I've also added a bunch of test points as apparently their very useful for programming and debugging. I realised I needed a button for the Fido2 user presence verification so I implemented a 4 pin SMD button thats also quite cheap and good, heres the [LCSC page](https://www.lcsc.com/product-detail/C318893.html). 

I was looking over decoupling things and overall polish I noticed I needed decoupling for the Security element and a bulk cap for the Vbus. I'm still figuring out whether I need 22 Ohm damping resistors as I feel its a bit unnecessary so I will do more research later on.

--------------------------------------------------------------------------------------------- 

![image](https://cdn.hackclub.com/01a002dd-46ed-73e6-b779-f8578ee98214/Screenshot%20from%202026-08-15%2001-33-11.png)


## Final schematic polish and working on bom + footprint assignment - Day 4 --- 1.9 Hours

Welp, I'm pretty much done with the schematic I had a look into a bunch of stuff today, first of all I added 22 Ohm resistors for the USB-A connector. They seem to be worth it and much safer! 

I made a quick 5 minute logo on canva which I might remake (?) along with organised it I will try to customise the borders more tomorrow to make it a good fit. 

The footprint assignment and BOM (Bill of materials) research has begun! I've figured out exact LCSC part numbers for the Resistors, Capacitors and the LED along with footprints. pro tip: once you get a supplier for small parts like this stick with them as shipping from 5 different suppliers will be more expensive than the value of the components sob. 

I tried ordering from LCSC, Mouser and Digikey later realised shipping would absolutely break me 💔 

I'll stick to 0603 components as I fear 0402 will be incredibly hard to solder, PCBA cost's a ton so I will be using a 15$ hotplate I got to reflow it with a stencil at home. 

Overall I worked on polishing everything, changed the pinout for the voltage regulator to match my choice for purchase and made it much more readable! 

---------------------------------------------------------------------------------------------

![image](https://cdn.hackclub.com/01a00829-702a-75cb-a8c7-31e2ea6c473d/Screenshot%20from%202026-08-16%2002-06-28.png)

![image](https://cdn.hackclub.com/01a00829-8cf7-78a1-8e6e-0120d7912806/Screenshot%20from%202026-08-16%2002-01-10.png)

![image](https://user-cdn.hackclub-assets.com/01a00829-8dd3-746f-b101-cccc5406d3a0/Screenshot%20from%202026-08-16%2002-01-25.png)


