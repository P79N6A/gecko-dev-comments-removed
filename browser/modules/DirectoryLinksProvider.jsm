



"use strict";

this.EXPORTED_SYMBOLS = ["DirectoryLinksProvider"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const XMLHttpRequest =
  Components.Constructor("@mozilla.org/xmlextras/xmlhttprequest;1", "nsIXMLHttpRequest");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
  "resource://gre/modules/NewTabUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyGetter(this, "gTextDecoder", () => {
  return new TextDecoder();
});


const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_LINKS_TYPE = "application/json";


const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";


const PREF_SELECTED_LOCALE = "general.useragent.locale";


const PREF_DIRECTORY_SOURCE = "browser.newtabpage.directory.source";


const PREF_DIRECTORY_PING = "browser.newtabpage.directory.ping";


const PREF_NEWTAB_ENHANCED = "browser.newtabpage.enhanced";


const ALLOWED_FRECENT_SITES = new Map([
  [ 'airdroid.com,android-developers.blogspot.com,android.com,androidandme.com,androidapplications.com,androidapps.com,androidauthority.com,androidcentral.com,androidcommunity.com,androidfilehost.com,androidforums.com,androidguys.com,androidheadlines.com,androidpit.com,androidpolice.com,androidspin.com,androidtapp.com,androinica.com,droid-life.com,droidforums.net,droidviews.com,droidxforums.com,forum.xda-developers.com,phandroid.com,play.google.com,shopandroid.com,talkandroid.com,theandroidsoul.com,thedroidguy.com,videodroid.org',
    'Technology' ],
  [ 'assurancewireless.com,att.com,attsavings.com,boostmobile.com,budgetmobile.com,consumercellular.com,credomobile.com,gosmartmobile.com,h2owirelessnow.com,lycamobile.com,lycamobile.us,metropcs.com,myfamilymobile.com,polarmobile.com,qlinkwireless.com,republicwireless.com,sprint.com,straighttalk.com,t-mobile.com,tracfonewireless.com,verizonwireless.com,virginmobile.com,virginmobile.com.au,virginmobileusa.com,vodafone.co.uk,vodafone.com,vzwshop.com',
    'Mobile Phone' ],
  [ 'addons.mozilla.org,air.mozilla.org,blog.mozilla.org,bugzilla.mozilla.org,developer.mozilla.org,etherpad.mozilla.org,forums.mozillazine.org,hacks.mozilla.org,hg.mozilla.org,mozilla.org,planet.mozilla.org,quality.mozilla.org,support.mozilla.org,treeherder.mozilla.org,wiki.mozilla.org',
    'Mozilla' ],
  [ '3dprint.com,4sysops.com,access-programmers.co.uk,accountingweb.com,addictivetips.com,adweek.com,afterdawn.com,akihabaranews.com,anandtech.com,appsrumors.com,arstechnica.com,belkin.com,besttechinfo.com,betanews.com,bgr.com,botcrawl.com,breakingmuscle.com,canonrumors.com,cheap-phones.com,chip.de,chip.eu,cio.com,citeworld.com,cleanpcremove.com,cnet.com,commentcamarche.net,computer.org,computerhope.com,computershopper.com,computerweekly.com,contextures.com,coolest-gadgets.com,crn.com,csoonline.com,daniweb.com,data.com,datacenterknowledge.com,ddj.com,devicemag.com,digitaltrends.com,dottech.org,dpreview.com,dslreports.com,edugeek.net,eetimes.com,engadget.com,epic.com,eurekalert.org,eweek.com,experts-exchange.com,extremetech.com,fosshub.com,freesoftwaremagazine.com,funkyspacemonkey.com,futuremark.com,gadgetreview.com,ghacks.net,gizmodo.co.uk,gizmodo.com,globalsecurity.org,greenbot.com,gunup.com,guru3d.com,head-fi.org,hexus.net,hothardware.com,howtoforge.com,idg.com.au,idigitaltimes.com,idownloadblog.com,ihackmyi.com,ilounge.com,infomine.com,informationweek.com,intellireview.com,intomobile.com,iphonehacks.com,ismashphone.com,isource.com,it168.com,itechpost.com,itpro.co.uk,itworld.com,jailbreaknation.com,kioskea.net,laptoping.com,laptopmag.com,lightreading.com,livescience.com,malwaretips.com,mediaroom.com,mobilemag.com,modmyi.com,modmymobile.com,mophie.com,mozillazine.org,neoseeker.com,neowin.net,newscientist.com,newsoxy.com,nextadvisor.com,notebookcheck.com,notebookreview.com,nvidia.com,nwc.com,orafaq.com,osdir.com,osxdaily.com,our-hometown.com,pcadvisor.co.uk,pchome.net,pcmag.com,pconline.com.cn,pcpop.com,pcpro.co.uk,pcreview.co.uk,pcrisk.com,pcwelt.de,phonerebel.com,phonescoop.com,physorg.com,pocket-lint.com,post-theory.com,prnewswire.co.uk,prnewswire.com,programming4.us,quickpwn.com,readwrite.com,redmondpie.com,redorbit.com,reviewed.com,safer-networking.org,sciencedaily.com,sciencenews.org,scientificamerican.com,scientificblogging.com,sciverse.com,servicerow.com,sinfuliphone.com,singularityhub.com,slashdot.org,slashgear.com,softonic.com,softonic.com.br,softonic.fr,sophos.com,space.com,sparkfun.com,speedguide.net,stuff.tv,techdailynews.net,techdirt.com,techeblog.com,techhive.com,techie-buzz.com,technewsworld.com,techniqueworld.com,technobuffalo.com,technologyreview.com,technologytell.com,techpowerup.com,techpp.com,techrepublic.com,techshout.com,techweb.com,techworld.com,techworld.com.au,techworldtweets.com,telecomfile.com,tgdaily.com,theinquirer.net,thenextweb.com,theregister.co.uk,thermofisher.com,theverge.com,thewindowsclub.com,tomsguide.com,tomshardware.com,tomsitpro.com,toptenreviews.com,trustedreviews.com,tuaw.com,tweaktown.com,ubergizmo.com,unwiredview.com,venturebeat.com,wccftech.com,webmonkey.com,webpronews.com,windows7codecs.com,windowscentral.com,windowsitpro.com,windowstechies.com,winsupersite.com,wired.co.uk,wired.com,wp-themes.com,xda-developers.com,xml.com,zdnet.com,zmescience.com,zol.com.cn',
    'Technology' ],
  [ '9to5mac.com,appadvice.com,apple.com,appleinsider.com,appleturns.com,appsafari.com,cultofmac.com,everymac.com,insanelymac.com,iphoneunlockspot.com,isource.com,itunes.apple.com,lowendmac.com,mac-forums.com,macdailynews.com,macenstein.com,macgasm.net,macintouch.com,maclife.com,macnews.com,macnn.com,macobserver.com,macosx.com,macpaw.com,macrumors.com,macsales.com,macstories.net,macupdate.com,macuser.co.uk,macworld.co.uk,macworld.com,maxiapple.com,spymac.com,theapplelounge.com',
    'Technology' ],
  [ 'alistapart.com,answers.microsoft.com,backpack.openbadges.org,blog.chromium.org,caniuse.com,codefirefox.com,codepen.io,css-tricks.com,css3generator.com,cssdeck.com,csswizardry.com,devdocs.io,docs.angularjs.org,ghacks.net,github.com,html5demos.com,html5rocks.com,html5test.com,iojs.org,khanacademy.org,l10n.mozilla.org,learn.jquery.com,marketplace.firefox.com,mozilla-hispano.org,mozillians.org,news.ycombinator.com,npmjs.com,packagecontrol.io,quirksmode.org,readwrite.com,reps.mozilla.org,smashingmagazine.com,stackoverflow.com,status.modern.ie,teamtreehouse.com,tutorialspoint.com,udacity.com,validator.w3.org,w3.org,w3cschool.cc,w3schools.com,whatcanidoformozilla.org',
    'Web Development' ],
  [ 'classroom.google.com,codecademy.com,elearning.ut.ac.id,khanacademy.org,learn.jquery.com,teamtreehouse.com,tutorialspoint.com,udacity.com,w3cschool.cc,w3schools.com',
    'Web Education' ],
  [ 'abebooks.co.uk,abebooks.com,alibris.com,allaboutcircuits.com,allyoucanbooks.com,answersingenesis.org,artnet.com,audiobooks.com,barnesandnoble.com,barnesandnobleinc.com,bartleby.com,betterworldbooks.com,biggerbooks.com,bncollege.com,bookbyte.com,bookdepository.com,bookfinder.com,bookrenter.com,booksamillion.com,booksite.com,boundless.com,brookstone.com,btol.com,calibre-ebook.com,campusbookrentals.com,casadellibro.com,cbomc.com,cengagebrain.com,chapters.indigo.ca,christianbook.com,ciscopress.com,coursesmart.com,cqpress.com,crafterschoice.com,crossings.com,cshlp.org,deseretbook.com,directtextbook.com,discountmags.com,doubledaybookclub.com,doubledaylargeprint.com,doverpublications.com,ebooks.com,ecampus.com,fellabooks.net,fictionwise.com,flatworldknowledge.com,grolier.com,harpercollins.com,hayhouse.com,historybookclub.com,hpb.com,hpbmarketplace.com,interweave.com,iseeme.com,katiekazoo.com,knetbooks.com,learnoutloud.com,librarything.com,literaryguild.com,lulu.com,lww.com,macmillan.com,magazines.com,mbsdirect.net,militarybookclub.com,mypearsonstore.com,mysteryguild.com,netplaces.com,noble.com,novelguide.com,onespirit.com,oxfordjournals.org,paperbackswap.com,papy.co.jp,peachpit.com,penguin.com,penguingroup.com,pimsleur.com,powells.com,qpb.com,quepublishing.com,reviews.com,rhapsodybookclub.com,rodalestore.com,royalsocietypublishing.org,sagepub.com,scrubsmag.com,sfbc.com,simonandschuster.com,simonandschuster.net,simpletruths.com,teach12.net,textbooks.com,textbookx.com,thegoodcook.com,thriftbooks.com,tlsbooks.com,toshibabookplace.com,tumblebooks.com,urbookdownload.com,valorebooks.com,valuemags.com,wwnorton.com,zoobooks.com',
    'Literature' ],
  [ 'aceshowbiz.com,aintitcoolnews.com,askkissy.com,askmen.com,atraf.co.il,audioboom.com,beamly.com,blippitt.com,bollywoodlife.com,bossip.com,buzzlamp.com,celebdirtylaundry.com,celebfocus.com,celebitchy.com,celebrity-gossip.net,celebrityabout.com,celebwild.com,chisms.net,concertboom.com,crushable.com,cultbox.co.uk,dailyentertainmentnews.com,dayscafe.com,deadline.com,deathandtaxesmag.com,diaryofahollywoodstreetking.com,digitalspy.com,egotastic.com,empirenews.net,enelbrasero.com,everydaycelebs.com,ew.com,extratv.com,facade.com,fanaru.com,fhm.com,geektyrant.com,glamourpage.com,heatworld.com,hlntv.com,hollyscoop.com,hollywoodreporter.com,hollywoodtuna.com,hypable.com,infotransfer.net,insideedition.com,interaksyon.com,jezebel.com,justjared.com,justjaredjr.com,komando.com,koreaboo.com,maxgo.com,maxim.com,maxviral.com,mediatakeout.com,mosthappy.com,moviestalk.com,my.ology.com,ngoisao.net,nofilmschool.com,nolocreo.com,octane.tv,ouchpress.com,people.com,peopleenespanol.com,perezhilton.com,pinkisthenewblog.com,platotv.tv,playbill.com,playbillvault.com,playgroundmag.net,popeater.com,popnhop.com,popsugar.co.uk,popsugar.com,purepeople.com,radaronline.com,rantchic.com,reshareworthy.com,rinkworks.com,ripbird.com,sara-freder.com,screenjunkies.com,soapcentral.com,soapoperadigest.com,sobadsogood.com,splitsider.com,starcasm.net,starpulse.com,straightfromthea.com,stupidcelebrities.net,stupiddope.com,tbn.org,theawesomedaily.com,theawl.com,thefrisky.com,thefw.com,theresacaputo.com,thezooom.com,tvnotas.com.mx,twanatells.com,vanswarpedtour.com,vietgiaitri.com,viral.buzz,vulture.com,wakavision.com,worthytales.net,wwtdd.com,younghollywood.com',
    'Entertainment News' ],
  [ '247wallst.com,4-traders.com,advfn.com,agweb.com,allbusiness.com,barchart.com,barrons.com,beckershospitalreview.com,benzinga.com,bizjournals.com,bizsugar.com,bloomberg.com,bloomberglaw.com,business-standard.com,businessinsider.com,businessinsider.com.au,businesspundit.com,businessweek.com,businesswire.com,cboe.com,cheatsheet.com,chicagobusiness.com,cjonline.com,cnbc.com,cnnmoney.com,cqrcengage.com,dailyfinance.com,dailyfx.com,dealbreaker.com,djindexes.com,dowjones.com,easierstreetdaily.com,economist.com,economyandmarkets.com,economywatch.com,edweek.org,eleconomista.es,entrepreneur.com,etfdailynews.com,etfdb.com,ewallstreeter.com,fastcolabs.com,fastcompany.com,financeformulas.net,financialpost.com,flife.de,forbes.com,forexpros.com,fortune.com,foxbusiness.com,ft.com,ftpress.com,fx-exchange.com,hbr.org,howdofinance.com,ibtimes.com,inc.com,investopedia.com,investors.com,investorwords.com,journalofaccountancy.com,kiplinger.com,lendingandcredit.net,lfb.org,mainstreet.com,markettraders.com,marketwatch.com,maxkeiser.com,minyanville.com,ml.com,moneycontrol.com,moneymappress.com,moneynews.com,moneysavingexpert.com,morningstar.com,mortgagenewsdaily.com,motleyfool.com,mt.co.kr,nber.org,nyse.com,oilprice.com,pewsocialtrends.org,principal.com,qz.com,rantfinance.com,realclearmarkets.com,recode.net,reuters.ca,reuters.co.in,reuters.co.uk,reuters.com,rttnews.com,seekingalpha.com,smallbiztrends.com,streetinsider.com,thecheapinvestor.com,theeconomiccollapseblog.com,themoneyconverter.com,thestreet.com,tickertech.com,tradingeconomics.com,updown.com,valuewalk.com,wikinvest.com,wsj.com,zacks.com',
    'Financial News' ],
  [ '10tv.com,8newsnow.com,9news.com,abc.net.au,abc7.com,abc7chicago.com,abcnews.go.com,aclu.org,activistpost.com,ajc.com,al.com,alan.com,alarab.net,aljazeera.com,americanthinker.com,app.com,aristeguinoticias.com,azcentral.com,baltimoresun.com,becomingminimalist.com,beforeitsnews.com,bigstory.ap.org,blackamericaweb.com,bloomberg.com,bloombergview.com,boston.com,bostonherald.com,breitbart.com,buffalonews.com,c-span.org,canada.com,cbs46.com,cbsnews.com,chicagotribune.com,chron.com,citizensvoice.com,citylab.com,cleveland.com,cnn.com,coed.com,countercurrentnews.com,courant.com,ctvnews.ca,dailyherald.com,dailynews.com,dallasnews.com,delawareonline.com,democratandchronicle.com,democraticunderground.com,democrats.org,denverpost.com,desmoinesregister.com,dispatch.com,elcomercio.pe,english.aljazeera.net,examiner.com,farsnews.com,firstcoastnews.com,firstpost.com,firsttoknow.com,foreignpolicy.com,foxnews.com,freebeacon.com,freep.com,fresnobee.com,gazette.com,global.nytimes.com,heraldtribune.com,hindustantimes.com,hngn.com,humanevents.com,huzlers.com,indiatimes.com,indystar.com,irishtimes.com,jacksonville.com,jpost.com,jsonline.com,kansascity.com,kctv5.com,kentucky.com,kickerdaily.com,king5.com,kmov.com,knoxnews.com,kpho.com,kvue.com,kwqc.com,kxan.com,lainformacion.com,latimes.com,ldnews.com,lex18.com,linternaute.com,livemint.com,lostateminor.com,m24.ru,macleans.ca,manchestereveningnews.co.uk,marinecorpstimes.com,masslive.com,mavikocaeli.com.tr,mcall.com,medium.com,mentalfloss.com,mercurynews.com,metro.us,miamiherald.com,militarytimes.com,mk.ru,mlive.com,mondotimes.com,montrealgazette.com,msnbc.com,msnewsnow.com,mynews13.com,mysanantonio.com,mysuncoast.com,nbclosangeles.com,nbcnewyork.com,nbcphiladelphia.com,ndtv.com,newindianexpress.com,news.cincinnati.com,news.google.com,news.msn.com,news.yahoo.com,news10.net,news8000.com,newsday.com,newsdaymarketing.net,newsen.com,newsmax.com,newsobserver.com,newsok.com,newsru.ua,newstatesman.com,newszoom.com,nj.com,nola.com,northjersey.com,nouvelobs.com,npr.org,nwfdailynews.com,nwitimes.com,nydailynews.com,nytimes.com,observer.com,ocregister.com,okcfox.com,omaha.com,onenewspage.com,ontheissues.org,oregonlive.com,orlandosentinel.com,palmbeachpost.com,pe.com,pennlive.com,philly.com,pilotonline.com,polar.com,post-gazette.com,postandcourier.com,presstelegram.com,presstv.ir,propublica.org,providencejournal.com,realclearpolitics.com,recorderonline.com,reporterdock.com,reporterherald.com,respublica.al,reuters.com,rg.ru,roanoke.com,sacbee.com,scmp.com,scnow.com,sdpnoticias.com,seattletimes.com,semana.com,sfgate.com,sharepowered.com,sinembargo.mx,slate.com,sltrib.com,sotomayortv.com,sourcewatch.org,spectator.co.uk,squaremirror.com,star-telegram.com,staradvertiser.com,startribune.com,statesman.com,stltoday.com,streetwise.co,stuff.co.nz,success.com,suffolknewsherald.com,sun-sentinel.com,sunnewsnetwork.ca,suntimes.com,supernewschannel.com,surenews.com,svoboda.org,syracuse.com,tampabay.com,tbd.com,telegram.com,telegraph.co.uk,tennessean.com,the-open-mind.com,theadvocate.com,theage.com.au,theatlantic.com,thebarefootwriter.com,theblaze.com,thecalifornian.com,thedailysheeple.com,thefix.com,theintelligencer.net,thelocal.com,thenational.ae,thenewstribune.com,theparisreview.org,thereporter.com,therepublic.com,thestar.com,thetelegram.com,thetimes.co.uk,theuspatriot.com,time.com,timescall.com,timesdispatch.com,timesleaderonline.com,timesofisrael.com,toledoblade.com,toprightnews.com,townhall.com,tpnn.com,trendolizer.com,triblive.com,tribune.com.pk,tricities.com,troymessenger.com,trueactivist.com,truthandaction.org,tsn.ua,tulsaworld.com,twincities.com,upi.com,usatoday.com,utsandiego.com,vagazette.com,viralwomen.com,vitalworldnews.com,voasomali.com,vox.com,washingtonexaminer.com,washingtonpost.com,watchdog.org,wave3.com,wavy.com,wbay.com,wbtw.com,wcpo.com,wctrib.com,wdtn.com,weeklystandard.com,westernjournalism.com,wfsb.com,wgrz.com,whas11.com,winonadailynews.com,wishtv.com,wistv.com,wkbn.com,wkow.com,wlfi.com,wmtw.com,wmur.com,wopular.com,world-top-news.com,worldnews.com,wplol.us,wpsdlocal6.com,wptz.com,wric.com,wsmv.com,wthitv.com,wthr.com,wtnh.com,wtol.com,wtsp.com,wvec.com,wwlp.com,wwltv.com,wyff4.com,yonhapnews.co.kr,yourbreakingnews.com',
    'News' ],
  [ '2k.com,360game.vn,4399.com,a10.com,activision.com,addictinggames.com,alawar.com,alienwarearena.com,anagrammer.com,andkon.com,aq.com,arcadeprehacks.com,arcadeyum.com,arcgames.com,archeagegame.com,armorgames.com,askmrrobot.com,battle.net,battlefieldheroes.com,bigfishgames.com,bigpoint.com,bioware.com,bluesnews.com,boardgamegeek.com,bollyheaven.com,bubblebox.com,bukkit.org,bungie.net,buycraft.net,callofduty.com,candystand.com,cda.pl,challonge.com,championselect.net,cheapassgamer.com,cheatcc.com,cheatengine.org,cheathappens.com,chess.com,civfanatics.com,clashofclans-tools.com,clashofclansbuilder.com,comdotgame.com,commonsensemedia.org,coolrom.com,crazygames.com,csgolounge.com,curse.com,d20pfsrd.com,destructoid.com,diablofans.com,diablowiki.net,didigames.com,dota2.com,dota2lounge.com,dressupgames.com,dulfy.net,ebog.com,elderscrollsonline.com,elitedangerous.com,elitepvpers.com,emuparadise.me,enjoydressup.com,escapegames24.com,escapistmagazine.com,eventhubs.com,eveonline.com,farming-simulator.com,feed-the-beast.com,flashgames247.com,flightrising.com,flipline.com,flonga.com,freegames.ws,freeonlinegames.com,fresh-hotel.org,friv.com,friv.today,fullypcgames.net,funny-games.biz,funtrivia.com,futhead.com,g2a.com,gamasutra.com,game-debate.com,game-oldies.com,game321.com,gamebaby.com,gamebaby.net,gamebanana.com,gamefaqs.com,gamefly.com,gamefront.com,gamegape.com,gamehouse.com,gameinformer.com,gamejolt.com,gamemazing.com,gamemeteor.com,gamerankings.com,gamersgate.com,games-msn.com,games-workshop.com,games.com,games2girls.com,gamesbox.com,gamesfreak.net,gametop.com,gametracker.com,gametrailers.com,gamezhero.com,gbatemp.net,geforce.com,gematsu.com,giantbomb.com,girl.me,girlsgames123.com,girlsplay.com,gog.com,gogames.me,gonintendo.com,goodgamestudios.com,gosugamers.net,greenmangaming.com,gtaforums.com,gtainside.com,guildwars2.com,hackedarcadegames.com,hearthpwn.com,hirezstudios.com,hitbox.tv,hltv.org,howrse.com,icy-veins.com,indiedb.com,jayisgames.com,jigzone.com,joystiq.com,juegosdechicas.com,kabam.com,kbhgames.com,kerbalspaceprogram.com,king.com,kixeye.com,kizi.com,kogama.com,kongregate.com,kotaku.com,lolcounter.com,lolking.net,lolnexus.com,lolpro.com,lolskill.net,lootcrate.com,lumosity.com,mafa.com,mangafox.me,mangapark.com,mariowiki.com,maxgames.com,megagames.com,metacritic.com,mindjolt.com,minecraft.net,minecraftforum.net,minecraftservers.org,minecraftskins.com,mineplex.com,miniclip.com,mmo-champion.com,mmobomb.com,mmohuts.com,mmorpg.com,mmosite.com,mobafire.com,moddb.com,modxvm.com,mojang.com,moshimonsters.com,mousebreaker.com,moviestarplanet.com,mtgsalvation.com,muchgames.com,myonlinearcade.com,myplaycity.com,myrealgames.com,mythicspoiler.com,n4g.com,newgrounds.com,nexon.net,nexusmods.com,ninjakiwi.com,nintendo.com,nintendoeverything.com,nintendolife.com,nitrome.com,nosteam.ro,notdoppler.com,noxxic.com,operationsports.com,origin.com,ownedcore.com,pacogames.com,pathofexile.com,pcgamer.com,pch.com,pcsx2.net,penny-arcade.com,planetminecraft.com,plarium.com,playdota.com,playpink.com,playsides.com,playstationlifestyle.net,playstationtrophies.org,pog.com,pokemon.com,polygon.com,popcap.com,primarygames.com,probuilds.net,ps3hax.net,psnprofiles.com,psu.com,qq.com,r2games.com,resourcepack.net,retrogamer.com,rewardtv.com,riotgames.com,robertsspaceindustries.com,roblox.com,robocraftgame.com,rockpapershotgun.com,rockstargames.com,roosterteeth.com,runescape.com,schoolofdragons.com,screwattack.com,scufgaming.com,segmentnext.com,shacknews.com,shockwave.com,shoryuken.com,siliconera.com,silvergames.com,skydaz.com,smashbros.com,solomid.net,starcitygames.com,starsue.net,steamcommunity.com,steamgifts.com,strategywiki.org,supercheats.com,surrenderat20.net,swtor.com,tankionline.com,tcgplayer.com,teamfortress.com,teamliquid.net,tetrisfriends.com,thesims3.com,thesimsresource.com,thetechgame.com,topg.org,totaljerkface.com,toucharcade.com,transformice.com,trueachievements.com,twcenter.net,twitch.tv,twoplayergames.org,unity3d.com,vg247.com,vgchartz.com,videogamesblogger.com,warframe.com,warlight.net,warthunder.com,watchcartoononline.com,websudoku.com,wildstar-online.com,wildtangent.com,wineverygame.com,wizards.com,worldofsolitaire.com,worldoftanks.com,wowhead.com,wowprogress.com,wowwiki.com,xbox.com,xbox360iso.com,xboxachievements.com,xfire.com,xtremetop100.com,y8.com,yoyogames.com,zybez.net,zynga.com',
    'Video Game' ],
]);


const ALLOWED_LINK_SCHEMES = new Set(["http", "https"]);


const ALLOWED_IMAGE_SCHEMES = new Set(["https", "data"]);


const DIRECTORY_FRECENCY = 1000;


const SUGGESTED_FRECENCY = Infinity;


const DEFAULT_FREQUENCY_CAP = 5;


const MIN_VISIBLE_HISTORY_TILES = 8;


const PING_SCORE_DIVISOR = 10000;


const PING_ACTIONS = ["block", "click", "pin", "sponsored", "sponsored_link", "unpin", "view"];






let DirectoryLinksProvider = {

  __linksURL: null,

  _observers: new Set(),

  
  _downloadDeferred: null,

  
  _downloadIntervalMS: 86400000,

  


  _enhancedLinks: new Map(),

  


  _frequencyCaps: new Map(),

  


  _suggestedLinks: new Map(),

  


  _topSitesWithSuggestedLinks: new Set(),

  get _observedPrefs() Object.freeze({
    enhanced: PREF_NEWTAB_ENHANCED,
    linksURL: PREF_DIRECTORY_SOURCE,
    matchOSLocale: PREF_MATCH_OS_LOCALE,
    prefSelectedLocale: PREF_SELECTED_LOCALE,
  }),

  get _linksURL() {
    if (!this.__linksURL) {
      try {
        this.__linksURL = Services.prefs.getCharPref(this._observedPrefs["linksURL"]);

        
        if (this.locale == "en-US" && !Services.prefs.prefHasUserValue(this._observedPrefs["linksURL"])) {
          this.__linksURL = "data:text/plain;base64,ewogICAgImRpcmVjdG9yeSI6IFsKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDQ5OCwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy9kMTFiYTBiMzA5NWJiMTlkODA5MmNkMjliZTljYmI5ZTE5NzY3MWVhLjI4MDg4LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzEzMzJhNjhiYWRmMTFlM2Y3ZjY5YmY3MzY0ZTc5YzBhN2UyNzUzYmMuNTMxNi5wbmciLAogICAgICAgICAgICAidGl0bGUiOiAiTW96aWxsYSBDb21tdW5pdHkiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHA6Ly9jb250cmlidXRlLm1vemlsbGEub3JnLyIKICAgICAgICB9LAogICAgICAgIHsKICAgICAgICAgICAgImJnQ29sb3IiOiAiIiwKICAgICAgICAgICAgImRpcmVjdG9yeUlkIjogNTAwLAogICAgICAgICAgICAiZW5oYW5jZWRJbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzL2NjNjM3NzRiN2E5YWFlMDJmZTM2YmM1Y2FmOTBjMWUyNWU2NmEyYmMuMTM3OTEucG5nIiwKICAgICAgICAgICAgImltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvZTgyMmNkNDYyOGM1MTYyMzEzZjQ5ZjVkNDU1NmY4YWFmZGYzODc1MC4xMTUxMy5wbmciLAogICAgICAgICAgICAidGl0bGUiOiAiTW96aWxsYSBNYW5pZmVzdG8iLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vd3d3Lm1vemlsbGEub3JnL2Fib3V0L21hbmlmZXN0by8iCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwMiwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy80MGU1NjMwNDA1ZDUwMzFjYTczMzkzYmQ3YmMwMDY0MTU2ZjJjYzgyLjEwOTg0LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzQ5MGQ0MmQxZjlhNzZjMDc3Mzk2MjZkMWI4YTU2OTE2OWFlYzhmYmUuMTEwMzkucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkN1c3RvbWl6ZSBGaXJlZm94IiwKICAgICAgICAgICAgInR5cGUiOiAiYWZmaWxpYXRlIiwKICAgICAgICAgICAgInVybCI6ICJodHRwOi8vZmFzdGVzdGZpcmVmb3guY29tL2ZpcmVmb3gvZGVza3RvcC9jdXN0b21pemUvIgogICAgICAgIH0sCiAgICAgICAgewogICAgICAgICAgICAiYmdDb2xvciI6ICIiLAogICAgICAgICAgICAiZGlyZWN0b3J5SWQiOiA1MDQsCiAgICAgICAgICAgICJlbmhhbmNlZEltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvODc3ZjFjNTYxZTczNWY3YjlmNDE5ZmY5YWM3OWViOGM3NDgxMTE5ZC4xNjc0NC5wbmciLAogICAgICAgICAgICAiaW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy8yNWM5ZmJiMDczMDhiODRkMTYwZmMxYjc5NTkzNjRhMmMxOGY5M2I5LjY0MDQucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkZpcmVmb3ggTWFya2V0cGxhY2UiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vbWFya2V0cGxhY2UuZmlyZWZveC5jb20vIgogICAgICAgIH0sCiAgICAgICAgewogICAgICAgICAgICAiYmdDb2xvciI6ICIjM2ZiNThlIiwKICAgICAgICAgICAgImRpcmVjdG9yeUlkIjogNTA1LAogICAgICAgICAgICAiZW5oYW5jZWRJbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzcyMDEyMWU3NDYyZDhjNzg2M2I0ZGQ4ZmE3YjVjMTA4OWI1ZjVmYjIuMzM4NjIucG5nIiwKICAgICAgICAgICAgImltYWdlVVJJIjogImh0dHBzOi8vZHRleDRrdmJwcG92dC5jbG91ZGZyb250Lm5ldC9pbWFnZXMvMGU2MDMxNjc1YTljNDkxZGQwYzY1ZTljNjdjZmJmNTRhNTg4MGYxNy4yMjk1LnN2ZyIsCiAgICAgICAgICAgICJ0aXRsZSI6ICJNb3ppbGxhIFdlYm1ha2VyIiwKICAgICAgICAgICAgInR5cGUiOiAiYWZmaWxpYXRlIiwKICAgICAgICAgICAgInVybCI6ICJodHRwczovL3dlYm1ha2VyLm9yZy8%2FdXRtX3NvdXJjZT1kaXJlY3RvcnktdGlsZXMmdXRtX21lZGl1bT1maXJlZm94LWJyb3dzZXIiCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwNiwKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy9kOTcxY2JhZmEwMzA5YTIwMWU1MThhY2RhYzRmMWVlNGRhYmM3ZWFhLjE1MTA5LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzL2I0YWRjNThkZDNjMDJkYTM1NTEwNDk3N2I5MTAyNTUwNjBjZmQ2ZDguMTAzNTAucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIkZpcmVmb3ggU3luYyIsCiAgICAgICAgICAgICJ0eXBlIjogImFmZmlsaWF0ZSIsCiAgICAgICAgICAgICJ1cmwiOiAiaHR0cDovL21vemlsbGEtZXVyb3BlLm9yZy9maXJlZm94L3N5bmMiCiAgICAgICAgfSwKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiIsCiAgICAgICAgICAgICJkaXJlY3RvcnlJZCI6IDUwNywKICAgICAgICAgICAgImVuaGFuY2VkSW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy8yMmZiODU2Y2Q1ODM2NTg1NWViNzI1YjE1NjVmMDhhNzI0NjRlMDM5LjE4NzE3LnBuZyIsCiAgICAgICAgICAgICJpbWFnZVVSSSI6ICJodHRwczovL2R0ZXg0a3ZicHBvdnQuY2xvdWRmcm9udC5uZXQvaW1hZ2VzLzA2OGUwY2NiZDg3MDFhMjhlMmYwNzhjNjQwZWUwNzJiOWExNmUyZTEuMTI0OTAucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIlByaXZhY3kgUHJpbmNpcGxlcyIsCiAgICAgICAgICAgICJ0eXBlIjogImFmZmlsaWF0ZSIsCiAgICAgICAgICAgICJ1cmwiOiAiaHR0cDovL2V1cm9wZS5tb3ppbGxhLm9yZy9wcml2YWN5L3lvdSIKICAgICAgICB9CiAgICBdLAogICAgInN1Z2dlc3RlZCI6IFsKICAgICAgICB7CiAgICAgICAgICAgICJiZ0NvbG9yIjogIiNjYWUxZjQiLAogICAgICAgICAgICAiZGlyZWN0b3J5SWQiOiA3MDIsCiAgICAgICAgICAgICJmcmVjZW50X3NpdGVzIjogWwogICAgICAgICAgICAgICAgImFkZG9ucy5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiYWlyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJibG9nLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJidWd6aWxsYS5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiZGV2ZWxvcGVyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJldGhlcnBhZC5tb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAiaGFja3MubW96aWxsYS5vcmciLAogICAgICAgICAgICAgICAgImhnLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJtb3ppbGxhLm9yZyIsCiAgICAgICAgICAgICAgICAicGxhbmV0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJxdWFsaXR5Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJzdXBwb3J0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJzdXBwb3J0Lm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJ0cmVlaGVyZGVyLm1vemlsbGEub3JnIiwKICAgICAgICAgICAgICAgICJ3aWtpLm1vemlsbGEub3JnIgogICAgICAgICAgICBdLAogICAgICAgICAgICAiaW1hZ2VVUkkiOiAiaHR0cHM6Ly9kdGV4NGt2YnBwb3Z0LmNsb3VkZnJvbnQubmV0L2ltYWdlcy85ZWUyYjI2NTY3OGYyNzc1ZGUyZTRiZjY4MGRmNjAwYjUwMmU2MDM4LjM4NzUucG5nIiwKICAgICAgICAgICAgInRpdGxlIjogIlRoYW5rcyBmb3IgdGVzdGluZyEiLAogICAgICAgICAgICAidHlwZSI6ICJhZmZpbGlhdGUiLAogICAgICAgICAgICAidXJsIjogImh0dHBzOi8vd3d3Lm1vemlsbGEuY29tL2ZpcmVmb3gvdGlsZXMiCiAgICAgICAgfQogICAgXQp9Cg%3D%3D";
        }
      }
      catch (e) {
        Cu.reportError("Error fetching directory links url from prefs: " + e);
      }
    }
    return this.__linksURL;
  },

  



  get locale() {
    let matchOS;
    try {
      matchOS = Services.prefs.getBoolPref(PREF_MATCH_OS_LOCALE);
    }
    catch (e) {}

    if (matchOS) {
      return Services.locale.getLocaleComponentForUserAgent();
    }

    try {
      let locale = Services.prefs.getComplexValue(PREF_SELECTED_LOCALE,
                                                  Ci.nsIPrefLocalizedString);
      if (locale) {
        return locale.data;
      }
    }
    catch (e) {}

    try {
      return Services.prefs.getCharPref(PREF_SELECTED_LOCALE);
    }
    catch (e) {}

    return "en-US";
  },

  


  _setDefaultEnhanced: function DirectoryLinksProvider_setDefaultEnhanced() {
    if (!Services.prefs.prefHasUserValue(PREF_NEWTAB_ENHANCED)) {
      let enhanced = true;
      try {
        
        if (Services.prefs.getBoolPref("privacy.donottrackheader.enabled")) {
          enhanced = false;
        }
      }
      catch(ex) {}
      Services.prefs.setBoolPref(PREF_NEWTAB_ENHANCED, enhanced);
    }
  },

  observe: function DirectoryLinksProvider_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      switch (aData) {
        
        case this._observedPrefs.enhanced:
          this._setDefaultEnhanced();
          break;

        case this._observedPrefs.linksURL:
          delete this.__linksURL;
          

        
        case this._observedPrefs.matchOSLocale:
        case this._observedPrefs.prefSelectedLocale:
          this._fetchAndCacheLinksIfNecessary(true);
          break;
      }
    }
  },

  _addPrefsObserver: function DirectoryLinksProvider_addObserver() {
    for (let pref in this._observedPrefs) {
      let prefName = this._observedPrefs[pref];
      Services.prefs.addObserver(prefName, this, false);
    }
  },

  _removePrefsObserver: function DirectoryLinksProvider_removeObserver() {
    for (let pref in this._observedPrefs) {
      let prefName = this._observedPrefs[pref];
      Services.prefs.removeObserver(prefName, this);
    }
  },

  _cacheSuggestedLinks: function(link) {
    if (!link.frecent_sites || "sponsored" == link.type) {
      
      return;
    }
    for (let suggestedSite of link.frecent_sites) {
      let suggestedMap = this._suggestedLinks.get(suggestedSite) || new Map();
      suggestedMap.set(link.url, link);
      this._suggestedLinks.set(suggestedSite, suggestedMap);
    }
  },

  _fetchAndCacheLinks: function DirectoryLinksProvider_fetchAndCacheLinks(uri) {
    
    uri = uri.replace("%LOCALE%", this.locale);
    uri = uri.replace("%CHANNEL%", UpdateChannel.get());

    let deferred = Promise.defer();
    let xmlHttp = new XMLHttpRequest();

    let self = this;
    xmlHttp.onload = function(aResponse) {
      let json = this.responseText;
      if (this.status && this.status != 200) {
        json = "{}";
      }
      OS.File.writeAtomic(self._directoryFilePath, json, {tmpPath: self._directoryFilePath + ".tmp"})
        .then(() => {
          deferred.resolve();
        },
        () => {
          deferred.reject("Error writing uri data in profD.");
        });
    };

    xmlHttp.onerror = function(e) {
      deferred.reject("Fetching " + uri + " results in error code: " + e.target.status);
    };

    try {
      xmlHttp.open("GET", uri);
      
      xmlHttp.overrideMimeType(DIRECTORY_LINKS_TYPE);
      
      xmlHttp.setRequestHeader("Content-Type", DIRECTORY_LINKS_TYPE);
      xmlHttp.send();
    } catch (e) {
      deferred.reject("Error fetching " + uri);
      Cu.reportError(e);
    }
    return deferred.promise;
  },

  



  _fetchAndCacheLinksIfNecessary: function DirectoryLinksProvider_fetchAndCacheLinksIfNecessary(forceDownload=false) {
    if (this._downloadDeferred) {
      
      return this._downloadDeferred.promise;
    }

    if (forceDownload || this._needsDownload) {
      this._downloadDeferred = Promise.defer();
      this._fetchAndCacheLinks(this._linksURL).then(() => {
        
        this._lastDownloadMS = Date.now();
        this._downloadDeferred.resolve();
        this._downloadDeferred = null;
        this._callObservers("onManyLinksChanged")
      },
      error => {
        this._downloadDeferred.resolve();
        this._downloadDeferred = null;
        this._callObservers("onDownloadFail");
      });
      return this._downloadDeferred.promise;
    }

    
    return Promise.resolve();
  },

  


  get _needsDownload () {
    
    if ((Date.now() - this._lastDownloadMS) > this._downloadIntervalMS) {
      return true;
    }
    return false;
  },

  





  _readDirectoryLinksFile: function DirectoryLinksProvider_readDirectoryLinksFile() {
    let emptyOutput = {directory: [], suggested: [], enhanced: []};
    return OS.File.read(this._directoryFilePath).then(binaryData => {
      let output;
      try {
        let json = gTextDecoder.decode(binaryData);
        let linksObj = JSON.parse(json);
        output = {directory: linksObj.directory || [],
                  suggested: linksObj.suggested || [],
                  enhanced:  linksObj.enhanced  || []};
      }
      catch (e) {
        Cu.reportError(e);
      }
      return output || emptyOutput;
    },
    error => {
      Cu.reportError(error);
      return emptyOutput;
    });
  },

  






  reportSitesAction: function DirectoryLinksProvider_reportSitesAction(sites, action, triggeringSiteIndex) {
    
    if (action == "view") {
      sites.slice(0, triggeringSiteIndex + 1).forEach(site => {
        let {targetedSite, url} = site.link;
        if (targetedSite) {
          this._decreaseFrequencyCap(url, 1);
        }
      });
    }
    
    else if (action == "click") {
      let {targetedSite, url} = sites[triggeringSiteIndex].link;
      if (targetedSite) {
        this._decreaseFrequencyCap(url, DEFAULT_FREQUENCY_CAP);
      }
    }

    let newtabEnhanced = false;
    let pingEndPoint = "";
    try {
      newtabEnhanced = Services.prefs.getBoolPref(PREF_NEWTAB_ENHANCED);
      pingEndPoint = Services.prefs.getCharPref(PREF_DIRECTORY_PING);
    }
    catch (ex) {}

    
    let invalidAction = PING_ACTIONS.indexOf(action) == -1;
    if (!newtabEnhanced || pingEndPoint == "" || invalidAction) {
      return Promise.resolve();
    }

    let actionIndex;
    let data = {
      locale: this.locale,
      tiles: sites.reduce((tiles, site, pos) => {
        
        if (site) {
          
          let {link} = site;
          let tilesIndex = tiles.length;
          if (triggeringSiteIndex == pos) {
            actionIndex = tilesIndex;
          }

          
          let id = link.directoryId;
          tiles.push({
            id: id || site.enhancedId,
            pin: site.isPinned() ? 1 : undefined,
            pos: pos != tilesIndex ? pos : undefined,
            score: Math.round(link.frecency / PING_SCORE_DIVISOR) || undefined,
            url: site.enhancedId && "",
          });
        }
        return tiles;
      }, []),
    };

    
    if (actionIndex !== undefined) {
      data[action] = actionIndex;
    }

    
    let ping = new XMLHttpRequest();
    ping.open("POST", pingEndPoint + (action == "view" ? "view" : "click"));
    ping.send(JSON.stringify(data));

    
    return this._fetchAndCacheLinksIfNecessary();
  },

  


  getEnhancedLink: function DirectoryLinksProvider_getEnhancedLink(link) {
    
    return link.enhancedImageURI && link ? link :
           this._enhancedLinks.get(NewTabUtils.extractSite(link.url));
  },

  



  getFrecentSitesName(sites) {
    return ALLOWED_FRECENT_SITES.get(sites.join(","));
  },

  


  isURLAllowed: function DirectoryLinksProvider_isURLAllowed(url, allowed) {
    
    if (!url) {
      return true;
    }

    let scheme = "";
    try {
      
      scheme = Services.io.newURI(url, null, null).scheme;
    }
    catch(ex) {}
    return allowed.has(scheme);
  },

  



  getLinks: function DirectoryLinksProvider_getLinks(aCallback) {
    this._readDirectoryLinksFile().then(rawLinks => {
      
      this._enhancedLinks.clear();
      this._frequencyCaps.clear();
      this._suggestedLinks.clear();

      let validityFilter = function(link) {
        
        return this.isURLAllowed(link.url, ALLOWED_LINK_SCHEMES) &&
               this.isURLAllowed(link.imageURI, ALLOWED_IMAGE_SCHEMES) &&
               this.isURLAllowed(link.enhancedImageURI, ALLOWED_IMAGE_SCHEMES);
      }.bind(this);

      rawLinks.suggested.filter(validityFilter).forEach((link, position) => {
        
        let name = this.getFrecentSitesName(link.frecent_sites);
        if (name == undefined) {
          return;
        }

        link.targetedName = name;
        link.lastVisitDate = rawLinks.suggested.length - position;

        
        
        this._cacheSuggestedLinks(link);
        this._frequencyCaps.set(link.url, DEFAULT_FREQUENCY_CAP);
      });

      rawLinks.enhanced.filter(validityFilter).forEach((link, position) => {
        link.lastVisitDate = rawLinks.enhanced.length - position;

        
        if (link.enhancedImageURI) {
          this._enhancedLinks.set(NewTabUtils.extractSite(link.url), link);
        }
      });

      let links = rawLinks.directory.filter(validityFilter).map((link, position) => {
        link.lastVisitDate = rawLinks.directory.length - position;
        link.frecency = DIRECTORY_FRECENCY;
        return link;
      });

      
      this.maxNumLinks = links.length + 1;

      return links;
    }).catch(ex => {
      Cu.reportError(ex);
      return [];
    }).then(links => {
      aCallback(links);
      this._populatePlacesLinks();
    });
  },

  init: function DirectoryLinksProvider_init() {
    this._setDefaultEnhanced();
    this._addPrefsObserver();
    
    this._directoryFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, DIRECTORY_LINKS_FILE);
    this._lastDownloadMS = 0;

    NewTabUtils.placesProvider.addObserver(this);
    NewTabUtils.links.addObserver(this);

    return Task.spawn(function() {
      
      let doesFileExists = yield OS.File.exists(this._directoryFilePath);
      if (doesFileExists) {
        let fileInfo = yield OS.File.stat(this._directoryFilePath);
        this._lastDownloadMS = Date.parse(fileInfo.lastModificationDate);
      }
      
      yield this._fetchAndCacheLinksIfNecessary();
    }.bind(this));
  },

  _handleManyLinksChanged: function() {
    this._topSitesWithSuggestedLinks.clear();
    this._suggestedLinks.forEach((suggestedLinks, site) => {
      if (NewTabUtils.isTopPlacesSite(site)) {
        this._topSitesWithSuggestedLinks.add(site);
      }
    });
    this._updateSuggestedTile();
  },

  




  _handleLinkChanged: function(aLink) {
    let changedLinkSite = NewTabUtils.extractSite(aLink.url);
    let linkStored = this._topSitesWithSuggestedLinks.has(changedLinkSite);

    if (!NewTabUtils.isTopPlacesSite(changedLinkSite) && linkStored) {
      this._topSitesWithSuggestedLinks.delete(changedLinkSite);
      return true;
    }

    if (this._suggestedLinks.has(changedLinkSite) &&
        NewTabUtils.isTopPlacesSite(changedLinkSite) && !linkStored) {
      this._topSitesWithSuggestedLinks.add(changedLinkSite);
      return true;
    }
    return false;
  },

  _populatePlacesLinks: function () {
    NewTabUtils.links.populateProviderCache(NewTabUtils.placesProvider, () => {
      this._handleManyLinksChanged();
    });
  },

  onLinkChanged: function (aProvider, aLink) {
    
    setTimeout(() => {
      if (this._handleLinkChanged(aLink) || this._shouldUpdateSuggestedTile()) {
        this._updateSuggestedTile();
      }
    }, 0);
  },

  onManyLinksChanged: function () {
    
    setTimeout(() => {
      this._handleManyLinksChanged();
    }, 0);
  },

  




  _decreaseFrequencyCap(url, amount) {
    let remainingViews = this._frequencyCaps.get(url) - amount;
    this._frequencyCaps.set(url, remainingViews);

    
    if (remainingViews <= 0) {
      this._updateSuggestedTile();
    }
  },

  _getCurrentTopSiteCount: function() {
    let visibleTopSiteCount = 0;
    for (let link of NewTabUtils.links.getLinks().slice(0, MIN_VISIBLE_HISTORY_TILES)) {
      if (link && (link.type == "history" || link.type == "enhanced")) {
        visibleTopSiteCount++;
      }
    }
    return visibleTopSiteCount;
  },

  _shouldUpdateSuggestedTile: function() {
    let sortedLinks = NewTabUtils.getProviderLinks(this);

    let mostFrecentLink = {};
    if (sortedLinks && sortedLinks.length) {
      mostFrecentLink = sortedLinks[0]
    }

    let currTopSiteCount = this._getCurrentTopSiteCount();
    if ((!mostFrecentLink.targetedSite && currTopSiteCount >= MIN_VISIBLE_HISTORY_TILES) ||
        (mostFrecentLink.targetedSite && currTopSiteCount < MIN_VISIBLE_HISTORY_TILES)) {
      
      
      
      
      
      return true;
    }

    return false;
  },

  





  _updateSuggestedTile: function() {
    let sortedLinks = NewTabUtils.getProviderLinks(this);

    if (!sortedLinks) {
      
      
      return;
    }

    
    let initialLength = sortedLinks.length;
    if (initialLength) {
      let mostFrecentLink = sortedLinks[0];
      if (mostFrecentLink.targetedSite) {
        this._callObservers("onLinkChanged", {
          url: mostFrecentLink.url,
          frecency: SUGGESTED_FRECENCY,
          lastVisitDate: mostFrecentLink.lastVisitDate,
          type: mostFrecentLink.type,
        }, 0, true);
      }
    }

    if (this._topSitesWithSuggestedLinks.size == 0 || !this._shouldUpdateSuggestedTile()) {
      
      
      return;
    }

    
    
    
    
    
    let possibleLinks = new Map();
    let targetedSites = new Map();
    this._topSitesWithSuggestedLinks.forEach(topSiteWithSuggestedLink => {
      let suggestedLinksMap = this._suggestedLinks.get(topSiteWithSuggestedLink);
      suggestedLinksMap.forEach((suggestedLink, url) => {
        
        if (this._frequencyCaps.get(url) <= 0) {
          return;
        }

        possibleLinks.set(url, suggestedLink);

        
        
        if (!targetedSites.get(url)) {
          targetedSites.set(url, []);
        }
        targetedSites.get(url).push(topSiteWithSuggestedLink);
      })
    });

    
    let numLinks = possibleLinks.size;
    if (numLinks == 0) {
      return;
    }

    let flattenedLinks = [...possibleLinks.values()];

    
    let suggestedIndex = Math.floor(Math.random() * numLinks);
    let chosenSuggestedLink = flattenedLinks[suggestedIndex];

    
    this._callObservers("onLinkChanged", Object.assign({
      frecency: SUGGESTED_FRECENCY,

      
      
      
      targetedSite: targetedSites.get(chosenSuggestedLink.url).length ?
        targetedSites.get(chosenSuggestedLink.url)[0] : null
    }, chosenSuggestedLink));
    return chosenSuggestedLink;
   },

  


  reset: function DirectoryLinksProvider_reset() {
    delete this.__linksURL;
    this._removePrefsObserver();
    this._removeObservers();
  },

  addObserver: function DirectoryLinksProvider_addObserver(aObserver) {
    this._observers.add(aObserver);
  },

  removeObserver: function DirectoryLinksProvider_removeObserver(aObserver) {
    this._observers.delete(aObserver);
  },

  _callObservers(methodName, ...args) {
    for (let obs of this._observers) {
      if (typeof(obs[methodName]) == "function") {
        try {
          obs[methodName](this, ...args);
        } catch (err) {
          Cu.reportError(err);
        }
      }
    }
  },

  _removeObservers: function() {
    this._observers.clear();
  }
};
