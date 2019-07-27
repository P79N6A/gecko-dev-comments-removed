



"use strict";

this.EXPORTED_SYMBOLS = ["DirectoryLinksProvider"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const ParserUtils =  Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils);

Cu.importGlobalProperties(["XMLHttpRequest"]);

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
XPCOMUtils.defineLazyServiceGetter(this, "eTLD",
  "@mozilla.org/network/effective-tld-service;1",
  "nsIEffectiveTLDService");
XPCOMUtils.defineLazyGetter(this, "gTextDecoder", () => {
  return new TextDecoder();
});
XPCOMUtils.defineLazyGetter(this, "gCryptoHash", function () {
  return Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
});
XPCOMUtils.defineLazyGetter(this, "gUnicodeConverter", function () {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = 'utf8';
  return converter;
});



const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_LINKS_TYPE = "application/json";


const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";


const PREF_SELECTED_LOCALE = "general.useragent.locale";


const PREF_DIRECTORY_SOURCE = "browser.newtabpage.directory.source";


const PREF_DIRECTORY_PING = "browser.newtabpage.directory.ping";


const PREF_NEWTAB_ENHANCED = "browser.newtabpage.enhanced";


const ALLOWED_FRECENT_SITES = new Map([
  [ '1800petmeds.com,800petmeds.com,adopt.dogtime.com,adoptapet.com,akc.org,americanhumane.org,animal.discovery.com,animalconcerns.org,animalshelter.org,arcatapet.com,aspca.org,avma.org,bestfriends.org,blog.petmeds.com,buddydoghs.com,carealotpets.com,dailypuppy.com,dog.com,dogbar.com,dogbreedinfo.com,drsfostersmith.com,entirelypets.com,farmsanctuary.org,farmusa.org,freekibble.com,freekibblekat.com,healthypets.com,hsus.org,humanesociety.org,liveaquaria.com,marinedepot.com,medi-vet.com,nationalpetpharmacy.com,nsalamerica.org,nycacc.org,ohmydogsupplies.com,pet-dog-cat-supply-store.com,petcarerx.com,petco.com,petdiscounters.com,petedge.com,peteducation.com,petfinder.com,petfooddirect.com,petguys.com,petharbor.com,petmountain.com,petplanet.co.uk,pets911.com,petsmart.com,petsuppliesplus.com,puppyfind.com,revivalanimal.com,terrificpets.com,thatpetplace.com,theanimalrescuesite.com,theanimalrescuesite.greatergood.com,thefluffingtonpost.com,therainforestsite.com,vetdepot.com',
    'pet' ],
  [ '1aauto.com,autoblog.com,autoguide.com,autosite.com,autoweek.com,bimmerpost.com,bmwblog.com,boldride.com,caranddriver.com,carcomplaints.com,carspoon.com,cherokeeforum.com,classiccars.com,commercialtrucktrader.com,corvetteforum.com,dealerrater.com,ebizautos.com,ford-trucks.com,hemmings.com,jalopnik.com,jeepforum.com,jeepsunlimited.com,jk-forum.com,legendaryspeed.com,motorauthority.com,motortrend.com,motorwings.com,odometer.com,pirate4x4.com,purecars.com,roadandtrack.com,teslamotorsclub.com,topgear.com,topspeed.com,totalmini.com,truckpaper.com,wranglerforum.com',
    'auto' ],
  [ 'autobytel.com,autocheck.com,automotive.com,autonation.com,autos.aol.com,autos.msn.com,autos.yahoo.com,autotrader.autos.msn.com,autotrader.com,autotraderclassics.com,autoweb.com,car.com,carbuyingtips.com,carfax.com,cargurus.com,carmax.com,carprices.com,cars.com,cars.oodle.com,carsdirect.com,carsforsale.com,edmunds.com,hertzcarsales.com,imotors.com,intellichoice.com,internetautoguide.com,kbb.com,lemonfree.com,nada.com,nadaguides.com,thecarconnection.com,thetruthaboutcars.com,truecar.com,usedcars.com,usnews.rankingsandreviews.com',
    'auto' ],
  [ 'acura.com,audi.ca,audi.com,audiusa.com,automobiles.honda.com,bentleymotors.com,bmw.com,bmwusa.com,buick.com,buyatoyota.com,cadillac.com,cars.mclaren.com,chevrolet.com,choosenissan.com,chrysler.com,daimler.com,dodge.com,ferrari.com/en_us,fiskerautomotive.com,ford.com,gm.com,gmc.com,hummer.com,hyundai.com,hyundaiusa.com,infiniti.com,infinitiusa.com,jaguarusa.com,jeep.com,kia.com,kiamotors.com,lamborghini.com/en/home,landrover.com,landroverusa.com,lexus.com,lincoln.com,maserati.us,mazda.com,mazdausa.com,mbusa.com,mbusi.com,mercedes-amg.com,mercedes-benz.com,mercuryvehicles.com,miniusa.com,nissanusa.com,pontiac.com,porsche.com/usa,ramtrucks.com,rolls-roycemotorcars.com,saturn.com,scion.com,subaru.com,teslamotors.com,toyota.com,volkswagen.co.uk,volkswagen.com,volvocars.com/us,vw.com',
    'auto' ],
  [ '1010tires.com,4wheelparts.com,advanceautoparts.com,andysautosport.com,autoanything.com,autogeek.net,autopartsgiant.com,autopartswarehouse.com,autotrucktoys.com,autozone.com,autozoneinc.com,bavauto.com,bigotires.com,bilsteinus.com,brembo.com,car-part.com,carid.com,carparts.com,carquest.com,dinancars.com,discounttire.com,discounttiredirect.com,firestonecompleteautocare.com,goodyear.com,hrewheels,jcwhitney.com,kw-suspensions.com,momousa.com,napaonline.com,onlinetires.com,oreillyauto.com,oriellysautoparts.com,pepboys.com,repairpal.com,rockauto.com,shop.advanceautoparts.com,slickcar.com,stoptech.com,streetbeatcustoms.com,summitracing.com,tirebuyer.com,tirerack.com,tiresplus.com,tsw.com,velocitymotoring.com,wheelmax.com',
    'auto parts' ],
  [ 'abebooks.co.uk,abebooks.com,addall.com,alibris.com,allaboutcircuits.com,allbookstores.com,allyoucanbooks.com,answersingenesis.org,artnet.com,audiobooks.com,barnesandnoble.com,barnesandnobleinc.com,bartleby.com,betterworldbooks.com,biblio.com,biggerbooks.com,bncollege.com,bookbyte.com,bookdepository.com,bookfinder.com,bookrenter.com,booksamillion.com,booksite.com,boundless.com,brookstone.com,btol.com,calibre-ebook.com,campusbookrentals.com,casadellibro.com,cbomc.com,cengagebrain.com,chapters.indigo.ca,christianbook.com,ciscopress.com,coursesmart.com,cqpress.com,crafterschoice.com,crossings.com,cshlp.org,deseretbook.com,directtextbook.com,discountmags.com,doubledaybookclub.com,doubledaylargeprint.com,doverpublications.com,ebooks.com,ecampus.com,fellabooks.net,fictionwise.com,flatworldknowledge.com,goodreads.com,grolier.com,harpercollins.com,hayhouse.com,historybookclub.com,hpb.com,hpbmarketplace.com,interweave.com,iseeme.com,katiekazoo.com,knetbooks.com,learnoutloud.com,librarything.com,literaryguild.com,lulu.com,lww.com,macmillan.com,magazines.com,mbsdirect.net,militarybookclub.com,mypearsonstore.com,mysteryguild.com,netplaces.com,noble.com,novelguide.com,onespirit.com,oxfordjournals.org,paperbackswap.com,papy.co.jp,peachpit.com,penguin.com,penguingroup.com,pimsleur.com,powells.com,qpb.com,quepublishing.com,reviews.com,rhapsodybookclub.com,rodalestore.com,royalsocietypublishing.org,sagepub.com,scrubsmag.com,sfbc.com,simonandschuster.com,simonandschuster.net,simpletruths.com,teach12.net,textbooks.com,textbookx.com,thegoodcook.com,thriftbooks.com,tlsbooks.com,toshibabookplace.com,tumblebooks.com,urbookdownload.com,usedbooksearch.co.uk,valorebooks.com,valuemags.com,vialibri.net,wwnorton.com,zoobooks.com',
    'literature' ],
  [ '53.com,ally.com,bankofamerica.com,bbt.com,bnymellon.com,capitalone.com/bank/,chase.com,citi.com,citibank.com,citizensbank.com,citizensbankonline.com,creditonebank.com,everbank.com,hsbc.com,key.com,pnc.com,pncbank.com,rbs.co.uk,regions.com,sovereignbank.com,suntrust.com,tdbank.com,usaa.com,usbank.com,wachovia.com,wamu.com,wellsfargo.com,wsecu.org',
    'banking' ],
  [ '247wallst.com,bizjournals.com,bloomberg.com,businessweek.com,cnbc.com,cnnmoney.com,dowjones.com,easyhomesite.com,economist.com,entrepreneur.com,fastcompany.com,finance.yahoo.com,forbes.com,fortune.com,foxbusiness.com,ft.com,hbr.org,ibtimes.com,inc.com,manta.com,marketwatch.com,newsweek.com,online.wsj.com,qz.com,reuters.com,smartmoney.com,wsj.com',
    'business news' ],
  [ 'achievecard.com,americanexpress.com,barclaycardus.com,card.com,citicards.com,comparecards.com,creditcards.citi.com,discover.com,discovercard.com,experian.com,skylightpaycard.com,squareup.com,visa.com,visabuxx.com,visaextras.com',
    'finance' ],
  [ 'alliantcreditunion.org,connexuscu.org,lmcu.org,nasafcu.com,navyfcu.org,navyfederal.org,penfed.org,sccu.com,suncoastcreditunion.com,tinkerfcu.org,veridiancu.org',
    'finance' ],
  [ 'allbusiness.com,bankrate.com,buyersellertips.com,cboe.com,cnbcprime.com,coindesk.com,dailyfinance.com,dailyfx.com,dealbreaker.com,easierstreetdaily.com,economywatch.com,etfdailynews.com,etfdb.com,financeformulas.net,finviz.com,fool.com,forexpros.com,forexthreads.com,ftpress.com,fx-exchange.com,insidermonkey.com,investmentu.com,investopedia.com,investorjunkie.com,investors.com,kiplinger.com,minyanville.com,moneymorning.com,moneyning.com,moneysavingexpert.com,morningstar.com,nakedcapitalism.com,ncsoft.net,oilprice.com,realclearmarkets.com,rttnews.com,seekingalpha.com,silverdoctors.com,stockcharts.com,stockpickr.com,thefinancials.com,thestreet.com,wallstreetinsanity.com,wikinvest.com,xe.com,youngmoney.com',
    'investing' ],
  [ 'edwardjones.com,fidelity.com,goldmansachs.com,jpmorgan.com,ml.com,morganstanley.com,mymerrill.com,personal.vanguard.com,principal.com,schwab.com,schwabplan.com,scottrade.com,tdameritrade.com,troweprice.com,vanguard.com',
    'investing' ],
  [ '247lendinggroup.com,americanoneunsecured.com,avant.com,bestegg.com,chasestudentloans.com,eloan.com,gofundme.com,guidetolenders.com,kiva.org,lendacademy.com,lendingclub.com,lendingtree.com,lightstream.com,loanio.com,manageyourloans.com,meetearnest.com,microplace.com,netcredit.com,peer-lend.com,personalloans.com,prosper.com,salliemae.com,sofi.com,springleaf.com,uk.zopa.com,upstart.com',
    'finance' ],
  [ 'betterment.com,blooom.com,futureadvisor.com,kapitall.com,motifinvesting.com,personalcapital.com,wealthfront.com,wisebanyan.com',
    'investing' ],
  [ 'bancdebinary.com,cherrytrade.com,empireoption.net,etrade.com,firstrade.com,forex.com,interactivebrokers.com,ishares.com,optionsxpress.com,sharebuilder.com,thinkorswim.com,tradeking.com,trademonster.com,us.etrade.com,zecco.com',
    'finance' ],
  [ 'annualcreditreport.com,bluebird.com,credio.com,creditkarma.com,creditreport.com,cybersource.com,equifax.com,freecreditreport.com,freecreditscore.com,freedomdebtrelief.com,freescoreonline.com,mint.com,moneymappress.com,myfico.com,nationaldebtrelief.com,onesmartpenny.com,paypal.com,transunion.com,truecredit.com,upromise.com,vuebill.com,xpressbillpay.com,youneedabudget.com',
    'personal finance' ],
  [ 'angieslist.com,bloomberg.com,businessinsider.com,buydomains.com,domain.com,entrepreneur.com,fastcompany.com,forbes.com,fortune.com,godaddy.com,inc.com,manta.com,nytimes.com,openforum.com,register.com,salesforce.com,sba.gov,sbomag.com,shopsmall.americanexpress.com,smallbusiness.yahoo.com,squarespace.com,startupjournal.com,startupnation.com,weebly.com,wordpress.com,youngentrepreneur.com',
    'business news' ],
  [ '1040now.net,24hourtax.com,acttax.com,comparetaxsoftware.org,e-file.com,etax.com,free1040taxreturn.com,hrblock.com,intuit.com,irstaxdoctors.com,libertytax.com,octaxcol.com,pay1040.com,priortax.com,quickbooks.com,quickrefunds.com,rapidtax.com,refundschedule.com,taxact.com,taxactonline.com,taxefile.com,taxhead.com,taxhelptoday.me,taxsimple.org,turbotax.com',
    'tax' ],
  [ 'adeccousa.com,americasjobexchange.com,aoljobs.com,applicantpro.com,applicantstack.com,apply-4-jobs.com,apply2jobs.com,att.jobs,beyond.com,careerboutique.com,careerbuilder.com,careerflash.net,careerslocal.net,climber.com,coverlettersandresume.com,dice.com,diversityonecareers.com,employmentguide.com,everyjobforme.com,experteer.com,find.ly,findtherightjob.com,freelancer.com,gigats.com,glassdoor.com,governmentjobs.com,hrapply.com,hrdepartment.com,hrsmart.com,ihire.com,indeed.com,internships.com,itsmycareer.com,job-applications.com,job-hunt.org,job-interview-site.com,job.com,jobcentral.com,jobdiagnosis.com,jobhat.com,jobing.com,jobrapido.com,jobs.aol.com,jobs.net,jobsbucket.com,jobsflag.com,jobsgalore.com,jobsonline.com,jobsradar.com,jobster.com,jobtorch.com,jobungo.com,jobvite.com,juju.com,linkedin.com,livecareer.com,localjobster.com,mindtools.com,monster.com,myjobhelper.com,myperfectresume.com,payscale.com,pryor.com,quintcareers.com,randstad.com,recruitingcenter.net,resume-library.com,resume-now.com,roberthalf.com,salary.com,salaryexpert.com,simplyhired.com,smartrecruiters.com,snagajob.com,startwire.com,theladders.com,themuse.com,theresumator.com,thingamajob.com,usajobs.gov,ziprecruiter.com',
    'career services' ],
  [ 'americanheart.org,americanredcross.com,americares.org,catholiccharitiesusa.org,charitybuzz.com,charitynavigator.org,charitywater.org,directrelief.org,fao.org,habitat.org,hrw.org,imf.org,mskcc.org,ohchr.org,redcross.org,reliefweb.int,salvationarmyusa.org,savethechildren.org,un.org,undp.org,unep.org,unesco.org,unfpa.org,unhcr.org,unicef.org,unicefusa.org,unops.org,volunteermatch.org,wfp.org,who.int,worldbank.org',
    'philanthropic' ],
  [ 'academia.edu,albany.edu,american.edu,amity.edu,annauniv.edu,apus.edu,arizona.edu,ashford.edu,asu.edu,auburn.edu,austincc.edu,baylor.edu,bc.edu,berkeley.edu,brandeis.edu,brookings.edu,brown.edu,bu.edu,buffalo.edu,byu.edu,calpoly.edu,calstate.edu,caltech.edu,cam.ac.uk,cambridge.org,capella.edu,case.edu,clemson.edu,cmu.edu,colorado.edu,colostate-pueblo.edu,colostate.edu,columbia.edu,commnet.edu,cornell.edu,cpp.edu,csulb.edu,csun.edu,csus.edu,cuny.edu,cwru.edu,dartmouth.edu,depaul.edu,devry.edu,drexel.edu,du.edu,duke.edu,emory.edu,fau.edu,fcps.edu,fiu.edu,fordham.edu,fsu.edu,fullerton.edu,fullsail.edu,gatech.edu,gcu.edu,georgetown.edu,gmu.edu,gsu.edu,gwu.edu,harvard.edu,hawaii.edu,hbs.edu,iastate.edu,iit.edu,illinois.edu,indiana.edu,iu.edu,jhu.edu,k-state.edu,kent.edu,ku.edu,lamar.edu,liberty.edu,losrios.edu,lsu.edu,luc.edu,maine.edu,maricopa.edu,mass.edu,miami.edu,miamioh.edu,missouri.edu,mit.edu,mnscu.edu,monash.edu,msu.edu,mtu.edu,nau.edu,ncsu.edu,nd.edu,neu.edu,njit.edu,northeastern.edu,northwestern.edu,nova.edu,nyu.edu,odu.edu,ohio-state.edu,ohio.edu,okstate.edu,oregonstate.edu,osu.edu,ou.edu,ox.ac.uk,pdx.edu,pearson.com,phoenix.edu,pitt.edu,princeton.edu,psu.edu,purdue.edu,regis.edu,rice.edu,rit.edu,rochester.edu,rpi.edu,rutgers.edu,sc.edu,scu.edu,sdsu.edu,seattleu.edu,sfsu.edu,si.edu,sjsu.edu,snhu.edu,stanford.edu,stonybrook.edu,suny.edu,syr.edu,tamu.edu,temple.edu,towson.edu,ttu.edu,tufts.edu,ua.edu,uark.edu,ub.edu,uc.edu,uccs.edu,ucdavis.edu,ucf.edu,uchicago.edu,uci.edu,ucla.edu,uconn.edu,ucr.edu,ucsb.edu,ucsc.edu,ucsd.edu,ucsf.edu,udel.edu,udemy.com,ufl.edu,uga.edu,uh.edu,uic.edu,uillinois.edu,uiowa.edu,uiuc.edu,uky.edu,umass.edu,umb.edu,umbc.edu,umd.edu,umich.edu,umn.edu,umuc.edu,unc.edu,uncc.edu,unf.edu,uniminuto.edu,universityofcalifornia.edu,unl.edu,unlv.edu,unm.edu,unt.edu,uoc.edu,uoregon.edu,upc.edu,upenn.edu,upi.edu,uri.edu,usc.edu,usf.edu,usg.edu,usu.edu,uta.edu,utah.edu,utdallas.edu,utexas.edu,utk.edu,uvm.edu,uw.edu,uwm.edu,vanderbilt.edu,vccs.edu,vcu.edu,virginia.edu,vt.edu,waldenu.edu,washington.edu,wayne.edu,wednet.edu,wgu.edu,wisc.edu,wisconsin.edu,wm.edu,wmich.edu,wsu.edu,wustl.edu,wvu.edu,yale.edu',
    'college' ],
  [ 'collegeboard.com,collegeconfidential.com,collegeview.com,ecollege.com,finaid.org,find-colleges-now.com,ratemyprofessors.com,ratemyteachers.com,studentsreview.com',
    'college' ],
  [ 'actstudent.org,adaptedmind.com,aesoponline.com,archives.com,bibme.org,blackboard.com,bookrags.com,cengage.com,chegg.com,classdojo.com,classzone.com,cliffsnotes.com,coursecompass.com,educationconnection.com,educationdynamics.com,ets.org,familysearch.org,fastweb.com,genealogy.com,gradesaver.com,instructure.com,khanacademy.org,learn4good.com,mathway.com,mathxl.com,mcgraw-hill.com,merriam-webster.com,mheducation.com,niche.com,openstudy.com,pearsoned.com,pearsonmylabandmastering.com,pearsonsuccessnet.com,poptropica.com,powerschool.com,proprofs.com,purplemath.com,quizlet.com,readwritethink.org,renlearn.com,rhymezone.com,schoolloop.com,schoology.com,smithsonianmag.com,sparknotes.com,study.com,studyisland.com,studymode.com,synonym.com,teacherprobs.com,teacherspayteachers.com,tutorvista.com,vocabulary.com,yourschoolmatch.com',
    'education' ],
  [ 'browardschools.com,k12.ca.us,k12.fl.us,k12.ga.us,k12.in.us,k12.mn.us,k12.mo.us,k12.nc.us,k12.nj.us,k12.oh.us,k12.va.us,k12.wi.us',
    'education' ],
  [ 'coolmath-games.com,coolmath.com,coolmath4kids.com,coolquiz.com,funbrain.com,funtrivia.com,gamesforthebrain.com,girlsgogames.com,hoodamath.com,lumosity.com,math.com,mathsisfun.com,trivia.com,wizard101.com',
    'learning games' ],
  [ 'askmen.com,boredomtherapy.com,buzzfeed.com,complex.com,dailymotion.com,elitedaily.com,gawker.com,howstuffworks.com,instagram.com,madamenoire.com,polygon.com,ranker.com,rollingstone.com,ted.com,theblaze.com,thechive.com,thecrux.com,thedailybeast.com,thoughtcatalog.com,uproxx.com,upworthy.com,zergnet.com',
    'entertainment' ],
  [ '11points.com,7gid.com,adultswim.com,break.com,cheezburger.com,collegehumor.com,cracked.com,dailydawdle.com,damnlol.com,dumb.com,dumblaws.com,ebaumsworld.com,explosm.net,failblog.org,fun-gallery.com,funnygig.com,funnyjunk.com,funnymama.com,funnyordie.com,funnytear.com,funplus.com,glassgiant.com,goingviralposts.com,gorillamask.net,i-am-bored.com,icanhascheezburger.com,ifunny.com,imjussayin.co,inherentlyfunny.com,izismile.com,jokes.com,keenspot.com,knowyourmeme.com,laughstub.com,memebase.com,mememaker.net,metacafe.com,mylol.com,picslist.com,punoftheday.com,queendom.com,rajnikantvscidjokes.in,regretfulmorning.com,shareonfb.com,somethingawful.com,stupidvideos.com,superfunnyimages.com,thedailywh.at,theonion.com,tosh.comedycentral.com,uberhumor.com,welltimedphotos.com',
    'humor' ],
  [ 'air.tv,amctheatres.com,boxofficemojo.com,cinapalace.com,cinaplay.com,cinemablend.com,cinemark.com,cinematical.com,collider.com,comicbookmovie.com,comingsoon.net,crackle.com,denofgeek.us,dreamworks.com,empireonline.com,enstarz.com,fandango.com,filmschoolrejects.com,flickeringmyth.com,flixster.com,fullmovie2k.com,g2g.fm,galleryhip.com,hollywood.com,hollywoodreporter.com,iglomovies.com,imdb.com,indiewire.com,instantwatcher.com,joblo.com,kickass.to,kissdrama.net,marcustheatres.com,megashare9.com,moviefone.com,movieinsider.com,moviemistakes.com,moviepilot.com,movierulz.com,movies.com,movies.yahoo.com,movieseum.com,movietickets.com,movieweb.com,mrmovietimes.com,mymovieshub.com,netflix.com,onlinemovies.pro,pelis24.com,projectfreetv.ch,redbox.com,regmovies.com,repelis.tv,rogerebert.suntimes.com,ropeofsilicon.com,rottentomatoes.com,sidereel.com,slashfilm.com,solarmovie.is,starwars.com,superherohype.com,tcm.com,twomovies.us,variety.com,vimeo.com,viooz.ac,warnerbros.com,watchfree.to,wbredirect.com,youtubeonfire.com,zmovie.tw,zumvo.com',
    'movie' ],
  [ '1079ishot.com,2dopeboyz.com,8tracks.com,acdc.com,allaccess.com,allhiphop.com,allmusic.com,audiofanzine.com,audiomack.com,azlyrics.com,baeblemusic.com,bandsintown.com,billboard.com,brooklynvegan.com,brunomars.com,buzznet.com,cmt.com,coachella.com,consequenceofsound.net,contactmusic.com,countryweekly.com,dangerousminds.net,datpiff.com,ddotomen.com,diffuser.fm,directlyrics.com,djbooth.net,eventful.com,fireflyfestival.com,genius.com,guitartricks.com,harmony-central.com,hiphopdx.com,hiphopearly.com,hypem.com,idolator.com,iheart.com,jambase.com,kanyetothe.com,knue.com,lamusica.com,last.fm,livemixtapes.com,loudwire.com,lyricinterpretations.com,lyrics.net,lyricsbox.com,lyricsmania.com,lyricsmode.com,metal-archives.com,metrolyrics.com,mp3.com,mtv.co.uk,myspace.com,newnownext.com,noisetrade.com,okayplayer.com,pandora.com,phish.com,pigeonsandplanes.com,pitchfork.com,popcrush.com,radio.com,rap-up.com,rdio.com,reverbnation.com,revolvermag.com,rockhall.com,saavn.com,songlyrics.com,soundcloud.com,spin.com,spinrilla.com,spotify.com,stereogum.com,stereotude.com,talkbass.com,tasteofcountry.com,thebacklot.com,theboombox.com,theboot.com,thissongissick.com,tunesbaby.com,ultimate-guitar.com,ultimateclassicrock.com,vevo.com,vibe.com,vladtv.com,whosampled.com,wikibit.me,worldstarhiphop.com,wyrk.com,xxlmag.com',
    'music' ],
  [ 'aceshowbiz.com,aintitcoolnews.com,allkpop.com,askkissy.com,atraf.co.il,audioboom.com,beamly.com,beyondhollywood.com,blastr.com,blippitt.com,bollywoodlife.com,bossip.com,buzzlamp.com,buzzsugar.com,cambio.com,celebdirtylaundry.com,celebrity-gossip.net,celebuzz.com,chisms.net,comicsalliance.com,concertboom.com,crushable.com,cultbox.co.uk,dailyentertainmentnews.com,dayscafe.com,deadline.com,deathandtaxesmag.com,diaryofahollywoodstreetking.com,digitalspy.com,dlisted.com,egotastic.com,empirenews.net,enelbrasero.com,eonline.com,etonline.com,ew.com,extratv.com,facade.com,famousfix.com,fanaru.com,fanpop.com,fansshare.com,fhm.com,geektyrant.com,glamourpage.com,gossipcenter.com,gossipcop.com,heatworld.com,hlntv.com,hollyscoop.com,hollywoodlife.com,hollywoodtuna.com,hypable.com,infotransfer.net,insideedition.com,interaksyon.com,jezebel.com,justjared.buzznet.com,justjared.com,justjaredjr.com,komando.com,koreaboo.com,laineygossip.com,maxgo.com,maxim.com,maxviral.com,mediatakeout.com,mosthappy.com,moviestalk.com,my.ology.com,nationalenquirer.com,necolebitchie.com,ngoisao.net,nofilmschool.com,nolocreo.com,octane.tv,okmagazine.com,ouchpress.com,people.com,peopleenespanol.com,perezhilton.com,pinkisthenewblog.com,platotv.tv,playbill.com,playbillvault.com,playgroundmag.net,popsugar.com,purepeople.com,radaronline.com,rantchic.com,realitytea.com,reshareworthy.com,rinkworks.com,ripbird.com,sara-freder.com,screencrush.com,screenjunkies.com,soapcentral.com,soapoperadigest.com,sobadsogood.com,socialitelife.com,sourcefednews.com,splitsider.com,starcasm.net,starmagazine.com,starpulse.com,straightfromthea.com,stupiddope.com,tbn.org,theawesomedaily.com,theawl.com,thefrisky.com,thefw.com,thehollywoodgossip.com,theresacaputo.com,thesuperficial.com,thezooom.com,tmz.com,tvnotas.com.mx,twanatells.com,usmagazine.com,vanityfair.com,vanswarpedtour.com,vietgiaitri.com,viral.buzz,vulture.com,wakavision.com,worthytales.net,wwtdd.com',
    'entertainment news' ],
  [ 'abc.go.com,abcfamily.go.com,abclocal.go.com,accesshollywood.com,aetv.com,amctv.com,animalplanet.com,bbcamerica.com,bet.com,biography.com,bravotv.com,cartoonnetwork.com,cbn.com,cbs.com,cc.com,centrictv.com,cinemax.com,comedycentral.com,ctv.ca,cwtv.com,daytondailynews.com,drphil.com,dsc.discovery.com,fox.com,fox23.com,fox4news.com,fxnetworks.com,hbo.com,history.com,hulu.com,ifc.com,iqiyi.com,jeopardy.com,kfor.com,logotv.com,mtv.com,myfoxchicago.com,myfoxdc.com,myfoxmemphis.com,myfoxphilly.com,nbc.com,nbcchicago.com,oxygen.com,pbs.org,pbskids.org,rachaelrayshow.com,rtve.es,scifi.com,sho.com,showtimeanytime.com,spike.com,sundance.tv,syfy.com,tbs.com,teamcoco.com,telemundo.com,thedoctorstv.com,titantv.com,tlc.com,tlc.discovery.com,tnt.tv,tntdrama.com,tv.com,tvguide.com,tvseriesfinale.com,usanetwork.com,uvidi.com,vh1.com,viki.com,watchcartoononline.com,watchseries-online.ch,wetv.com,wheeloffortune.com,whio.com,wnep.com,wral.com,wtvr.com,xfinitytv.com,yidio.com',
    'TV show' ],
  [ 'americanhiking.org,appalachiantrail.org,canadiangeographic.ca,defenders.org,discovermagazine.com,discoveroutdoors.com,dsc.discovery.com,earthtouchnews.com,edf.org,epa.gov,ewg.org,fishngame.org,foe.org,fs.fed.us,geography.about.com,landtrustalliance.org,nationalgeographic.com,nature.com,nrdc.org,nwf.org,outdoorchannel.com,outdoors.org,seedmagazine.com,trcp.org,usda.gov,worldwildlife.org',
    'environment' ],
  [ 'abbreviations.com,abcmouse.com,abcya.com,achieve3000.com,ancestry.com,animaljam.com,babble.com,babycenter.com,babynamespedia.com,behindthename.com,bestmomstv.com,brainyquote.com,cafemom.com,citationmachine.net,clubpenguin.com,cutemunchkins.com,discoveryeducation.com,disney.com,easybib.com,education.com,enotes.com,everydayfamily.com,familyeducation.com,gamefaqs.com,greatschools.org,hrw.com,imvu.com,infoplease.com,itsybitsysteps.com,justmommies.com,k12.com,kidsactivitiesblog.com,mathwarehouse.com,mom.me,mom365.com,mommyshorts.com,momswhothink.com,momtastic.com,monsterhigh.com,myheritage.com,nameberry.com,nickmom.com,pampers.com,parenthood.com,parenting.com,parenting.ivillage.com,parents.com,parentsociety.com,raz-kids.com,regentsprep.org,scarymommy.com,scholastic.com,shmoop.com,softschools.com,spanishdict.com,starfall.com,thebump.com,thefreedictionary.com,thenest.com,thinkbabynames.com,todaysparent.com,webkinz.com,whattoexpect.com',
    'family' ],
  [ 'americangirl.com,barbie.com,barbiecollectibles.com,cartoonnetworkshop.com,chuckecheese.com,coloring.ws,disney.co.uk,disney.com.au,disney.go.com,disney.in,disneychannel-asia.com,disneyinternational.com,disneyjunior.com,disneylatino.com,disneyme.com,dltk-kids.com,dressupone.com,fantage.com,funbrainjr.com,hotwheels.com,icarly.com,kiwicrate.com,marvel.com,marvelkids.com,mattelgames.com,maxsteel.com,monkeyquest.com,nick-asia.com,nick.co.uk,nick.com,nick.tv,nickelodeon.com.au,nickjr.co.uk,nickjr.com,ninjakiwi.com,notdoppler.com,powerrangers.com,sciencekids.co.nz,search.disney.com,seventeen.com,teennick.com,theslap.com,yepi.com',
    'family' ],
  [ 'alabama.gov,archives.gov,bls.gov,ca.gov,cancer.gov,cdc.gov,census.gov,cia.gov,cms.gov,commerce.gov,ct.gov,delaware.gov,dhs.gov,doi.gov,dol.gov,dot.gov,ed.gov,eftps.gov,epa.gov,fbi.gov,fda.gov,fema.gov,flhsmv.gov,ftc.gov,ga.gov,georgia.gov,gpo.gov,hhs.gov,house.gov,hud.gov,illinois.gov,in.gov,irs.gov,justice.gov,ky.gov,loc.gov,louisiana.gov,maryland.gov,mass.gov,michigan.gov,mo.gov,nih.gov,nj.gov,nps.gov,ny.gov,nyc.gov,ohio.gov,ok.gov,opm.gov,oregon.gov,pa.gov,recreation.gov,sba.gov,sc.gov,sec.gov,senate.gov,state.fl.us,state.gov,state.il.us,state.ma.us,state.mi.us,state.mn.us,state.nc.us,state.ny.us,state.oh.us,state.pa.us,studentloans.gov,telldc.com,texas.gov,tn.gov,travel.state.gov,tsa.gov,usa.gov,uscis.gov,uscourts.gov,usda.gov,usdoj.gov,usembassy.gov,usgs.gov,utah.gov,va.gov,virginia.gov,wa.gov,whitehouse.gov,wi.gov,wisconsin.gov',
    'government' ],
  [ 'beachbody.com,bodybuilding.com,caloriecount.com,extremefitness.com,fitbit.com,fitday.com,fitnessmagazine.com,fitnessonline.com,fitwatch.com,livestrong.com,maxworkouts.com,mensfitness.com,menshealth.com,muscleandfitness.com,muscleandfitnesshers.com,myfitnesspal.com,shape.com,womenshealthmag.com',
    'health & fitness' ],
  [ 'activebeat.com,alliancehealth.com,beyonddiet.com,caring.com,complete-health-and-happiness.com,diabeticconnect.com,doctoroz.com,everydayhealth.com,followmyhealth.com,greatist.com,health.com,healthboards.com,healthcaresource.com,healthgrades.com,healthguru.com,healthination.com,healthtap.com,helpguide.org,iherb.com,kidshealth.org,lifescript.com,lovelivehealth.com,medicaldaily.com,mercola.com,perfectorigins.com,prevention.com,qualityhealth.com,questdiagnostics.com,realself.com,sharecare.com,sparkpeople.com,spryliving.com,steadyhealth.com,symptomfind.com,ucomparehealthcare.com,vitals.com,webmd.com,weightwatchers.com,wellness.com,zocdoc.com',
    'health & wellness' ],
  [ 'aetna.com,anthem.com,athenahealth.com,bcbs.com,bluecrossca.com,cigna.benefitnation.net,cigna.com,cigna.healthplan.com,ehealthcare.com,ehealthinsurance.com,empireblue.com,goldenrule.com,healthcare.gov,healthnet.com,humana-medicare.com,humana.com,kaiserpermanente.org,metlife.com,my.cigna.com,mybenefits.metlife.com,myuhc.com,uhc.com,unitedhealthcareonline.com,walterrayholt.com',
    'health insurance' ],
  [ 'aafp.org,americanheart.org,apa.org,cancer.org,cancercenter.com,caremark.com,clevelandclinic.org,diabetesfree.org,drugs.com,emedicinehealth.com,express-scripts.com,familydoctor.org,goodrx.com,healthcaremagic.com,healthfinder.gov,healthline.com,ieee.org,intelihealth.com,labcorp.com,livecellresearch.com,mayoclinic.com,mayoclinic.org,md.com,medcohealth.com,medhelp.org,medicalnewstoday.com,medicare.gov,medicaresupplement.com,medicinenet.com,medscape.com,memorialhermann.org,merckmanuals.com,patient.co.uk,psychcentral.com,psychology.org,psychologytoday.com,rightdiagnosis.com,rxlist.com,socialpsychology.org,spine-health.com,who.int',
    'health & wellness' ],
  [ 'aaa.com,aig.com,allianz-assistance.com,allstate.com,allstateagencies.com,amfam.com,amica.com,autoquotesdirect.com,esurance.com,farmers.com,farmersagent.com,geico.com,general-car-insurance-quotes.net,insurance.com,libertymutual.com,libertymutualgroup.com,mercuryinsurance.com,nationwide.com,progressive.com,progressiveagent.com,progressiveinsurance.com,provide-insurance.com,safeco.com,statefarm.com,thehartford.com,travelers.com,usaa.com',
    'insurance' ],
  [ '101cookbooks.com,allrecipes.com,bettycrocker.com,bonappetit.com,chocolateandzucchini.com,chow.com,chowhound.chow.com,cookinglight.com,cooks.com,cooksillustrated.com,cooksrecipes.com,delish.com,eater.com,eatingwell.com,epicurious.com,food.com,foodandwine.com,foodgawker.com,foodnetwork.com,gourmet.com,grouprecipes.com,homemaderecipes.co,iheartnaptime.net,kraftfoods.com,kraftrecipes.com,myrecipes.com,opentable.com,pillsbury.com,recipe.com,recipesource.com,recipezaar.com,saveur.com,seriouseats.com,simplyrecipes.com,smittenkitchen.com,southernliving.com,supercook.com,tasteofhome.com,tastespotting.com,technicpack.net,thekitchn.com,urbanspoon.com,wonderhowto.com,yelp.com,yummly.com,zagat.com',
    'food & lifestyle' ],
  [ 'aarp.org,allure.com,bustle.com,cosmopolitan.com,diply.com,eharmony.com,elle.com,glamour.com,grandascent.com,harpersbazaar.com,hellogiggles.com,instructables.com,instyle.com,marieclaire.com,match.com,mindbodygreen.com,nymag.com,okcupid.com,petco.com,photobucket.com,pof.com,rantlifestyle.com,redbookmag.com,reddit.com,sheknows.com,style.com,stylebistro.com,theilovedogssite.com,theknot.com,thescene.com,thrillist.com,vogue.com,womansday.com,youngcons.com,yourdictionary.com',
    'lifestyle' ],
  [ 'apartmentratings.com,apartmenttherapy.com,architectmagazine.com,architecturaldigest.com,askthebuilder.com,bhg.com,bobvila.com,countryhome.com,countryliving.com,davesgarden.com,decor8blog.com,decorpad.com,diycozyhome.com,diyideas.com,diynetwork.com,doityourself.com,domainehome.com,dwell.com,elledecor.com,familyhandyman.com,frontdoor.com,gardenguides.com,gardenweb.com,getdecorating.com,goodhousekeeping.com,hgtv.com,hgtvgardens.com,hobbylobby.com,homeadvisor.com,homerepair.about.com,hometalk.com,hometime.com,hometips.com,housebeautiful.com,houzz.com,inhabitat.com,lonny.com,makingitlovely.com,marthastewart.com,michaels.com,myhomeideas.com,realsimple.com,remodelista.com,shanty-2-chic.com,styleathome.com,thehandmadehome.net,thehealthyhomeeconomist.com,thisoldhouse.com,traditionalhome.com,trulia.com,younghouselove.com',
    'home & lifestyle' ],
  [ '10best.com,10tv.com,11alive.com,19actionnews.com,9news.com,abcnews.com,abcnews.go.com,adweek.com,ajc.com,anchorfree.us,arcamax.com,austin360.com,azcentral.com,bbc.co.uk,boston.com,bostonglobe.com,capecodonline.com,cbsnews.com,cheatsheet.com,chicagotribune.com,chron.com,citylab.com,cnn.com,csmonitor.com,dailyitem.com,dailymail.co.uk,dallasnews.com,eleconomista.es,examiner.com,fastcolabs.com,fivethirtyeight.com,foursquare.com,foxcarolina.com,foxnews.com,globalnews.ca,greatergood.com,guardian.co.uk,historynet.com,huffingtonpost.co.uk,huffingtonpost.com,ijreview.com,independent.co.uk,journal-news.com,kare11.com,kcra.com,kctv5.com,kgw.com,khou.com,king5.com,kirotv.com,kitv.com,kmbc.com,knoxnews.com,kpho.com,kptv.com,kron4.com,ksdk.com,ksl.com,ktvb.com,kvue.com,kxan.com,latimes.com,lifehack.org,littlethings.com,mailtribune.com,mic.com,mirror.co.uk,msn.com,msnbc.com,msnbc.msn.com,myfoxboston.com,nbcnews.com,nbcnewyork.com,newburyportnews.com,news.bbc.co.uk,news.yahoo.com,news12.com,newschannel10.com,newsday.com,newser.com,newsmax.com,newyorker.com,nj.com,nj1015.com,npr.org,nydailynews.com,nypost.com,nytimes.com,palmbeachpost.com,patch.com,philly.com,phys.org,poconorecord.com,prnewswire.com,rare.us,realclearworld.com,record-eagle.com,richmond.com,rt.com,salemnews.com,salon.com,sfgate.com,slate.com,statesman.com,suntimes.com,takepart.com,telegraph.co.uk,theatlantic.com,thedailystar.com,theguardian.com,theroot.com,theverge.com,time.com,timesonline.co.uk,topix.com,usatoday.com,usatoday30.usatoday.com,usnews.com,vice.com,vox.com,wane.com,washingtonpost.com,washingtontimes.com,wave3.com,wavy.com,wbaltv.com,wbir.com,wcnc.com,wdbj7.com,westernjournalism.com,wfaa.com,wfsb.com,wftv.com,wgal.com,wishtv.com,wisn.com,wistv.com,wivb.com,wkyc.com,wlwt.com,wmur.com,woodtv.com,wpxi.com,wsbtv.com,wsfa.com,wsmv.com,wsoctv.com,wthr.com,wtnh.com,wtsp.com,wwltv.com,wyff4.com,wzzm13.com',
    'news' ],
  [ 'aei.org,breitbart.com,conservativetalknow.com,conservativetribune.com,dailykos.com,ddo.com,drudgereport.com,dscc.org,foreignpolicy.com,franklinprosperityreport.com,freedomworks.org,macleans.ca,mediamatters.org,militarytimes.com,nationaljournal.com,nationalreview.com,politicalwire.com,politico.com,pressrepublican.com,qpolitical.com,realclearpolitics.com,talkingpointsmemo.com,thehill.com,thenation.com,thinkprogress.org,tnr.com,worldoftanks.eu',
    'news' ],
  [ 'americanscientist.org,discovermagazine.com,iflscience.com,livescience.com,nasa.gov,nationalgeographic.com,nature.com,newscientist.com,popsci.com,sciencedaily.com,sciencemag.org,sciencenews.org,scientificamerican.com,space.com,zmescience.com',
    'science' ],
  [ 'accuweather.com,intellicast.com,noaa.gov,ssa.gov,theweathernetwork.com,weather.com,weather.gov,weather.yahoo.com,weatherbug.com,weatherunderground.com,weatherzone.com.au,wunderground.com,www.weather.com',
    'weather' ],
  [ 'bhphotovideo.com,bigfolio.com,bigstockphoto.com,cameralabs.com,canonrumors.com,canstockphoto.com,digitalcamerareview.com,dpreview.com,expertphotography.com,gettyimages.com,icp.org,imaging-resource.com,intothedarkroom.com,istockphoto.com,nikonusa.com,photos.com,shutterstock.com,slrgear.com,the-digital-picture.com,thephotoargus.com,usa.canon.com,whatdigitalcamera.com,zenfolio.com',
    'photography' ],
  [ 'abercrombie.com,ae.com,aeropostale.com,anthropologie.com,bananarepublic.com,buycostumes.com,chadwicks.com,express.com,forever21.com,freepeople.com,hm.com,hollisterco.com,jcrew.com,jessicalondon.com,kingsizemen.com,lordandtaylor.com,lulus.com,metrostyle.com,nomorerack.com,oldnavy.com,oldnavy.gap.com,polyvore.com,rackroomshoes.com,ralphlauren.com,refinery29.com,roamans.com,sammydress.com,shop.nordstrom.com,shopbop.com,topshop.com,urbanoutfitters.com,victoriassecret.com,wetseal.com,womanwithin.com',
    'shopping' ],
  [ 'bizrate.com,compare99.com,coupons.com,dealtime.com,epinions.com,junglee.com,kijiji.ca,pricegrabber.com,pronto.com,redplum.com,retailmenot.com,shopping.com,shopzilla.com,smarter.com,valpak.com',
    'shopping' ],
  [ '123greetings.com,1800baskets.com,1800flowers.com,americangreetings.com,birthdayexpress.com,bluemountain.com,e-cards.com,egreetings.com,florists.com,ftd.com,gifts.com,groupcard.com,harryanddavid.com,hipstercards.com,kabloom.com,personalcreations.com,proflowers.com,redenvelope.com,someecards.com',
    'flowers & gifts' ],
  [ '6pm.com,alibaba.com,aliexpress.com,amazon.co.uk,amazon.com,asos.com,bathandbodyworks.com,bloomingdales.com,bradsdeals.com,buy.com,cafepress.com,circuitcity.com,clarkhoward.com,consumeraffairs.com,costco.com,cvs.com,dhgate.com,diapers.com,dillards.com,ebates.com,ebay.com,ebaystores.com,etsy.com,fingerhut.com,groupon.com,hsn.com,jcpenney.com,kmart.com,kohls.com,kroger.com,lowes.com,macys.com,menards.com,nextag.com,nordstrom.com,orientaltrading.com,overstock.com,qvc.com,racked.com,rewardsnetwork.com,samsclub.com,sears.com,sephora.com,shopathome.com,shopify.com,shopstyle.com,slickdeals.net,soap.com,staples.com,target.com,toptenreviews.com,vistaprint.com,walgreens.com,walmart.ca,walmart.com,wayfair.com,zappos.com,zazzle.com,zulily.com',
    'shopping' ],
  [ 'acehardware.com,ashleyfurniture.com,bedbathandbeyond.com,brylanehome.com,casa.com,cb2.com,crateandbarrel.com,dwr.com,ethanallen.com,furniture.com,harborfreight.com,hayneedle.com,homedecorators.com,homedepot.com,ikea.com,info.ikea-usa.com,landofnod.com,pier1.com,plowhearth.com,potterybarn.com,restorationhardware.com,roomandboard.com,westelm.com,williams-sonoma.com',
    'home shopping' ],
  [ 'alexandermcqueen.com,bergdorfgoodman.com,bottegaveneta.com,burberry-bluelabel.com,burberry.com,chanel.com,christianlouboutin.com,coach.com,diesel.com,dior.com,dolcegabbana.com,dolcegabbana.it,fendi.com,ferragamo.com,giorgioarmani.com,givenchy.com,gucci.com,guess.com,hermes.com,jeanpaulgaultier.com,jimmychoo.com,juicycouture.com,katespade.com,louisvuitton.com,manoloblahnik.com,marcjacobs.com,neimanmarcus.com,net-a-porter.com,paulsmith.co.uk,prada.com,robertocavalli.com,saksfifthavenue.com,toryburch.com,valentino.com,versace.com,vuitton.com,ysl.com,yslbeautyus.com',
    'luxury shopping' ],
  [ 'bargainseatsonline.com,livenation.com,stubhub.com,ticketfly.com,ticketliquidator.com,ticketmaster.com,tickets.com,ticketsnow.com,ticketweb.com,vividseats.com',
    'events & tickets' ],
  [ 'babiesrus.com,brothers-brick.com,etoys.com,fao.com,fisher-price.com,hasbro.com,hasbrotoyshop.com,lego.com,legoland.com,mattel.com,toys.com,toysrus.com,toystogrowon.com,toywiz.com',
    'toys & games' ],
  [ 'challengegames.nfl.com,fantasy.nfl.com,fantasyfootballblog.net,fantasyfootballcafe.com,fantasyfootballnerd.com,fantasysmarts.com,fftoday.com,fftoolbox.com,football.fantasysports.yahoo.com,footballsfuture.com,mrfootball.com,officefootballpool.com,thehuddle.com',
    'fantasy football' ],
  [ 'dailyjoust.com,draftday.com,draftking.com,draftkings.com,draftstreet.com,fanduel.com,realmoneyfantasyleagues.com,thedailyaudible.com',
    'fantasy sports' ],
  [ 'cdmsports.com,fanball.com,fantasyguru.com,fantasynews.cbssports.com,fantasyquestions.com,fantasyrundown.com,fantasysharks.com,fantasysports.yahoo.com,fantazzle.com,fantrax.com,fleaflicker.com,junkyardjake.com,kffl.com,mockdraftcentral.com,myfantasyleague.com,rototimes.com,rotowire.com,rotoworld.com,rtsports.com,whatifsports.com',
    'fantasy sports' ],
  [ 'football.about.com,football.com,footballoutsiders.com,nationalfootballpost.com,nflalumni.org,nflpa.com,nfltraderumors.co,profootballhof.com,profootballtalk.com,profootballtalk.nbcsports.com,profootballweekly.com',
    'football' ],
  [ '49ers.com,atlantafalcons.com,azcardinals.com,baltimoreravens.com,bengals.com,buccaneers.com,buffalobills.com,chargers.com,chicagobears.com,clevelandbrowns.com,colts.com,dallascowboys.com,denverbroncos.com,detroitlions.com,giants.com,houstontexans.com,jaguars.com,kcchiefs.com,miamidolphins.com,neworleanssaints.com,newyorkjets.com,packers.com,panthers.com,patriots.com,philadelphiaeagles.com,raiders.com,redskins.com,seahawks.com,steelers.com,stlouisrams.com,titansonline.com,vikings.com',
    'football' ],
  [ 'baseball-reference.com,baseballamerica.com,europeantour.com,golf.com,golfdigest.com,lpga.com,milb.com,minorleagueball.com,mlb.com,mlb.mlb.com,nascar.com,nba.com,ncaa.com,nhl.com,pga.com,pgatour.com,prowrestling.com,surfermag.com,surfline.com,surfshot.com,thehockeynews.com,tsn.com,ufc.com,worldgolfchampionships.com,wwe.com',
    'sports' ],
  [ '247sports.com,active.com,armslist.com,basketball-reference.com,bigten.org,bleacherreport.com,bleedinggreennation.com,bloodyelbow.com,cagesideseats.com,cbssports.com,cinesport.com,collegespun.com,cricbuzz.com,crictime.com,csnphilly.com,csnwashington.com,cstv.com,eastbay.com,espn.com,espn.go.com,espncricinfo.com,espnfc.com,espnfc.us,espnradio.com,eteamz.com,fanatics.com,fansided.com,fbschedules.com,fieldandstream.com,flightclub.com,foxsports.com,givemesport.com,goduke.com,goheels.com,golfchannel.com,golfnow.com,grantland.com,grindtv.com,hoopshype.com,icc-cricket.com,imleagues.com,kentuckysportsradio.com,larrybrownsports.com,leaguelineup.com,maxpreps.com,mlbtraderumors.com,mmafighting.com,mmajunkie.com,mmamania.com,msn.foxsports.com,myscore.com,nbcsports.com,nbcsports.msnbc.com,nesn.com,rantsports.com,realclearsports.com,reserveamerica.com,rivals.com,runnersworld.com,sbnation.com,scout.com,sherdog.com,si.com,speedsociety.com,sportingnews.com,sports.yahoo.com,sportsillustrated.cnn.com,sportsmanias.com,sportsmonster.us,sportsonearth.com,stack.com,teamworkonline.com,thebiglead.com,thescore.com,trails.com,triblive.com,upickem.net,usatodayhss.com,watchcric.net,yardbarker.com',
    'sports news' ],
  [ 'adidas.com,backcountry.com,backcountrygear.com,cabelas.com,champssports.com,competitivecyclist.com,dickssportinggoods.com,finishline.com,footlocker.com,ladyfootlocker.com,modells.com,motosport.com,mountaingear.com,newbalance.com,nike.com,patagonia.com,puma.com,reebok.com,sportsmansguide.com,steepandcheap.com,tgw.com,thenorthface.com',
    'sports & outdoor goods' ],
  [ 'airdroid.com,android-developers.blogspot.com,android.com,androidandme.com,androidapplications.com,androidapps.com,androidauthority.com,androidcommunity.com,androidfilehost.com,androidforums.com,androidguys.com,androidheadlines.com,androidpit.com,androidspin.com,androidtapp.com,androinica.com,droid-life.com,droidforums.net,droidviews.com,droidxforums.com,forum.xda-developers.com,phandroid.com,play.google.com,shopandroid.com,talkandroid.com,theandroidsoul.com,thedroidguy.com,videodroid.org',
    'technology' ],
  [ '9to5mac.com,appadvice.com,apple.com,appleinsider.com,appleturns.com,appsafari.com,cultofmac.com,everymac.com,insanelymac.com,iphoneunlockspot.com,isource.com,itunes.apple.com,lowendmac.com,mac-forums.com,macdailynews.com,macenstein.com,macgasm.net,macintouch.com,maclife.com,macnews.com,macnn.com,macobserver.com,macosx.com,macpaw.com,macrumors.com,macsales.com,macstories.net,macupdate.com,macuser.co.uk,macworld.co.uk,macworld.com,maxiapple.com,spymac.com,theapplelounge.com',
    'technology' ],
  [ 'adobe.com,asus.com,avast.com,data.com,formstack.com,gboxapp.com,gotomeeting.com,hp.com,htc.com,ibm.com,intel.com,java.com,logme.in,mcafee.com,mcafeesecure.com,microsoftstore.com,norton.com,office.com,office365.com,opera.com,oracle.com,proboards.com,samsung.com,sourceforge.net,squarespace.com,techtarget.com,ultipro.com,uniblue.com,web.com,winzip.com',
    'technology' ],
  [ '3dprint.com,4sysops.com,access-programmers.co.uk,accountingweb.com,afterdawn.com,akihabaranews.com,appsrumors.com,avg.com,belkin.com,besttechinfo.com,betanews.com,botcrawl.com,breakingmuscle.com,cheap-phones.com,chip.de,chip.eu,citeworld.com,cleanpcremove.com,commentcamarche.net,computer.org,computerhope.com,computershopper.com,computerweekly.com,contextures.com,coolest-gadgets.com,csoonline.com,daniweb.com,datacenterknowledge.com,ddj.com,devicemag.com,digitaltrends.com,dottech.org,dslreports.com,edugeek.net,eetimes.com,epic.com,eurekalert.org,eweek.com,experts-exchange.com,fosshub.com,freesoftwaremagazine.com,funkyspacemonkey.com,futuremark.com,gadgetreview.com,gizmodo.co.uk,globalsecurity.org,gunup.com,guru3d.com,head-fi.org,hexus.net,hothardware.com,howtoforge.com,idg.com.au,idownloadblog.com,ihackmyi.com,ilounge.com,infomine.com,intellireview.com,intomobile.com,iphonehacks.com,ismashphone.com,it168.com,itechpost.com,itpro.co.uk,jailbreaknation.com,laptoping.com,lightreading.com,malwaretips.com,mediaroom.com,mobilemag.com,modmyi.com,modmymobile.com,mophie.com,mozillazine.org,neoseeker.com,neowin.net,newsoxy.com,nextadvisor.com,notebookcheck.com,notebookreview.com,nvidia.com,orafaq.com,osdir.com,osxdaily.com,our-hometown.com,pchome.net,pconline.com.cn,pcpop.com,pcpro.co.uk,pcreview.co.uk,pcrisk.com,pcwelt.de,phonerebel.com,phonescoop.com,physorg.com,pocket-lint.com,post-theory.com,prnewswire.co.uk,programming4.us,quickpwn.com,redmondpie.com,redorbit.com,safer-networking.org,scientificblogging.com,sciverse.com,servicerow.com,sinfuliphone.com,singularityhub.com,slashgear.com,softonic.com,softonic.com.br,softonic.fr,sophos.com,sparkfun.com,speedguide.net,stuff.tv,symantec.com,taplikahome.com,techdailynews.net,techeblog.com,techie-buzz.com,techniqueworld.com,technobuffalo.com,technologyreview.com,technologytell.com,techpowerup.com,techpp.com,techradar.com,techshout.com,techworld.com,techworld.com.au,techworldtweets.com,telecomfile.com,tgdaily.com,theinquirer.net,thenextweb.com,theregister.co.uk,thermofisher.com,thewindowsclub.com,tomsitpro.com,trustedreviews.com,tuaw.com,tweaktown.com,unwiredview.com,wccftech.com,webmonkey.com,webpronews.com,windows7codecs.com,windowscentral.com,windowsitpro.com,windowstechies.com,winsupersite.com,wired.co.uk,wp-themes.com,xml.com,zol.com.cn',
    'technology' ],
  [ 'addons.mozilla.org,air.mozilla.org,blog.mozilla.org,bugzilla.mozilla.org,developer.mozilla.org,etherpad.mozilla.org,forums.mozillazine.org,hacks.mozilla.org,hg.mozilla.org,mozilla.org,planet.mozilla.org,quality.mozilla.org,support.mozilla.org,treeherder.mozilla.org,wiki.mozilla.org',
    'Mozilla' ],
  [ 'addictivetips.com,allthingsd.com,anandtech.com,androidcentral.com,androidpolice.com,arstechnica.com,bgr.com,boygeniusreport.com,cio.com,cnet.com,computerworld.com,crn.com,electronista.com,engadget.com,extremetech.com,fastcocreate.com,fastcodesign.com,fastcoexist.com,frontlinek12.com,gigaom.com,gizmag.com,gizmodo.com,greenbot.com,howtogeek.com,idigitaltimes.com,imore.com,informationweek.com,infoworld.com,itworld.com,kioskea.net,laptopmag.com,leadpages.net,lifehacker.com,mashable.com,networkworld.com,news.cnet.com,nwc.com,pastebin.com,pcadvisor.co.uk,pcmag.com,pcworld.com,phonearena.com,reviewed.com,serverfault.com,siteadvisor.com,slashdot.org,techcrunch.com,techdirt.com,techhive.com,technewsworld.com,techrepublic.com,techweb.com,tomsguide.com,tomshardware.com,ubergizmo.com,venturebeat.com,wired.com,xda-developers.com,zdnet.com',
    'technology news' ],
  [ 'bestbuy.ca,bestbuy.com,cdw.com,compusa.com,computerlivehelp.co,cyberguys.com,dell.com,digitalinsight.com,directron.com,ebuyer.com,frontierpc.com,frys-electronics-ads.com,frys.com,geeks.com,gyazo.com,homestead.com,lenovo.com,macmall.com,microcenter.com,miniinthebox.com,mwave.com,newegg.com,officedepot.com,outletpc.com,outpost.com,radioshack.com,rakuten.com,tigerdirect.com',
    'tech retail' ],
  [ 'chat.com,fring.com,hello.firefox.com,oovoo.com,viber.com',
    'video chat' ],
  [ 'alistapart.com,answers.microsoft.com,backpack.openbadges.org,blog.chromium.org,caniuse.com,codefirefox.com,codepen.io,css-tricks.com,css3generator.com,cssdeck.com,csswizardry.com,devdocs.io,docs.angularjs.org,ghacks.net,github.com,html5demos.com,html5rocks.com,html5test.com,iojs.org,l10n.mozilla.org,marketplace.firefox.com,mozilla-hispano.org,mozillians.org,news.ycombinator.com,npmjs.com,packagecontrol.io,quirksmode.org,readwrite.com,reps.mozilla.org,smashingmagazine.com,speckyboy.com,stackoverflow.com,status.modern.ie,validator.w3.org,w3.org,webreference.com,whatcanidoformozilla.org',
    'web development' ],
  [ 'classroom.google.com,codeacademy.org,codecademy.com,codeschool.com,codeyear.com,elearning.ut.ac.id,how-to-build-websites.com,htmlcodetutorial.com,htmldog.com,htmlplayground.com,learn.jquery.com,quackit.com,roseindia.net,teamtreehouse.com,tizag.com,tutorialspoint.com,udacity.com,w3schools.com,webdevelopersnotes.com',
    'webdev education' ],
  [ 'att.com,att.net,attonlineoffers.com,bell.ca,bellsouth.com,cableone.net,cablevision.com,centurylink.com,centurylink.net,centurylinkquote.com,charter-business.com,charter.com,charter.net,chartercabledeals.com,chartermedia.com,comcast.com,comcast.net,cox.com,cox.net,coxnewsweb.com,directv.com,dish.com,dishnetwork.com,freeconferencecall.com,frontier.com,hughesnet.com,liveitwithcharter.com,mycenturylink.com,mydish.com,net10.com,officialtvstream.com.es,optimum.com,optimum.net,paygonline.com,paytm.com,qwest.com,rcn.com,rebtel.com,ringcentral.com,straighttalkbyop.com,swappa.com,textem.net,timewarner.com,timewarnercable.com,tracfone.com,verizon.com,verizon.net,voipo.com,vonagebusiness.com,wayport.net,whistleout.com,wildblue.net,windstream.net,windstreambusiness.net,wowway.com,ww2.cox.com,xfinity.com',
    'telecommunication' ],
  [ 'alltel.com,assurancewireless.com,attsavings.com,boostmobile.com,boostmobilestore.com,budgetmobile.com,consumercellular.com,credomobile.com,gosmartmobile.com,h2owirelessnow.com,lycamobile.com,lycamobile.us,metropcs.com,motorola.com,mycricket.com,myfamilymobile.com,nextel.com,nokia.com,nokiausa.com,polarmobile.com,qlinkwireless.com,republicwireless.com,sprint.com,sprintpcs.com,straighttalk.com,t-mobile.co.uk,t-mobile.com,tmobile.com,tracfonewireless.com,uscellular.com,verizonwireless.com,virginmobile.com,virginmobile.com.au,virginmobileusa.com,vodafone.co.uk,vodafone.com,vodaphone.co.uk,vonange.com,vzwshop.com,wireless.att.com',
    'mobile carrier' ],
  [ 'aa.com,aerlingus.com,airasia.com,aircanada.com,airfrance.com,airindia.com,alaskaair.com,alaskaairlines.com,allegiantair.com,britishairways.com,cathaypacific.com,china-airlines.com,continental.com,delta.com,deltavacations.com,dragonair.com,easyjet.com,elal.co.il,emirates.com,flightaware.com,flyfrontier.com,frontierairlines.com,hawaiianair.com,iberia.com,jetairways.com,jetblue.com,klm.com,koreanair.com,kuwait-airways.com,lan.com,lufthansa.com,malaysiaairlines.com,mihinlanka.com,nwa.com,qantas.com.au,qatarairways.com,ryanair.com,singaporeair.com,smartfares.com,southwest.com,southwestvacations.com,spiritair.com,spiritairlines.com,thaiair.com,united.com,usairways.com,virgin-atlantic.com,virginamerica.com,virginblue.com.au',
    'travel & airline' ],
  [ 'carnival.com,celebrity-cruises.com,celebritycruises.com,costacruise.com,cruise.com,cruiseamerica.com,cruisecritic.com,cruisedirect.com,cruisemates.com,cruises.com,cruisesonly.com,crystalcruises.com,cunard.com,disneycruise.disney.go.com,hollandamerica.com,ncl.com,pocruises.com,princess.com,royalcaribbean.com,royalcaribbean.cruiselines.com,rssc.com,seabourn.com,silversea.com,starcruises.com,vikingrivercruises.com,windstarcruises.com',
    'travel & cruise' ],
  [ 'agoda.com,airbnb.com,beaches.com,bedandbreakfast.com,bestwestern.com,booking.com,caesars.com,choicehotels.com,comfortinn.com,daysinn.com,dealbase.com,doubletree3.hilton.com,embassysuites.com,fairmont.com,flipkey.com,fourseasons.com,greatwolf.com,hamptoninn.hilton.com,hamptoninn3.hilton.com,hhonors3.hilton.com,hilton.com,hiltongardeninn3.hilton.com,hiltonworldwide.com,holidayinn.com,homeaway.com,hotelclub.com,hotelopia.com,hotels.com,hotelscombined.com,hyatt.com,ihg.com,laterooms.com,lhw.com,lq.com,mandarinoriental.com,marriott.com,motel6.com,omnihotels.com,radisson.com,ramada.com,rci.com,reservationcounter.com,resortvacationstogo.com,ritzcarlton.com,roomkey.com,sheraton.com,starwoodhotels.com,starwoodhotelshawaii.com,super8.com,thetrain.com,vacationhomerentals.com,vacationrentals.com,vrbo.com,wyndhamrewards.com',
    'hotel & resort' ],
  [ 'airfarewatchdog.com,airliners.net,atlanta-airport.com,budgettravel.com,cntraveler.com,cntraveller.com,destination360.com,flightstats.com,flyertalk.com,fodors.com,frommers.com,letsgo.com,lonelyplanet.com,matadornetwork.com,perfectvacation.co,ricksteves.com,roughguides.com,timeout.com,travelalberta.us,travelandleisure.com,travelchannel.com,traveler.nationalgeographic.com,travelmath.com,traveltune.com,tripadvisor.com,vegas.com,viator.com,virtualtourist.com,wikitravel.org,worldtravelguide.net',
    'travel' ],
  [ 'aavacations.com,applevacations.com,avianca.com,bookingbuddy.com,bookit.com,cheapair.com,cheapcaribbean.com,cheapflights.com,cheapoair.com,cheaptickets.com,chinahighlights.com,costcotravel.com,ctrip.com,despegar.com,edreams.net,expedia.ca,expedia.com,fareboom.com,farebuzz.com,farecast.live.com,farecompare.com,faregeek.com,flightnetwork.com,funjet.com,golastminute.com,hipmunk.com,hotwire.com,ifly.com,justairticket.com,kayak.com,lastminute.com,lastminutetravel.com,lowestfare.com,lowfares.com,momondo.com,onetime.com,onetravel.com,orbitz.com,otel.com,priceline.com,pricelinevisa.com,sidestep.com,skyscanner.com,smartertravel.com,statravel.com,tigerair.com,travelocity.com,travelonbids.com,travelzoo.com,tripsta.com,trivago.com,universalorlando.com,universalstudioshollywood.com,vacationexpress.com,venere.com,webjet.com,yatra.com',
    'travel' ],
  [ 'airportrentalcars.com,alamo.com,amtrak.com,anytransitguide.com,avis.com,boltbus.com,budget.com,carrentalexpress.com,carrentals.com,coachusa.com,dollar.com,e-zrentacar.com,enterprise.com,europcar.com,foxrentacar.com,gotobus.com,greyhound.com,hertz.com,hertzondemand.com,indianrail.gov.in,irctc.co.in,megabus.com,mta.info,nationalcar.com,nationalrail.co.uk,njtransit.com,paylesscar.com,paylesscarrental.com,peterpanbus.com,raileurope.com,rentalcars.com,rideuta.com,stagecoachbus.com,thrifty.com,uber.com,wanderu.com,zipcar.com',
    'travel & transit' ],
  [ 'bulbagarden.net,cheatcc.com,cheatmasters.com,cheats.ign.com,comicvine.com,computerandvideogames.com,counter-strike.net,escapistmagazine.com,gamedaily.com,gamefront.com,gameinformer.com,gamerankings.com,gamespot.com,gamesradar.com,gamestop.com,gametrailers.com,gamezone.com,giantbomb.com,ign.com,kotaku.com,metacritic.com,minecraft-server-list.com,minecraftforge.net,minecraftforum.net,minecraftservers.org,minecraftskins.com,mmo-champion.com,mojang.com,pcgamer.com,planetminecraft.com,supercheats.com,thesims.com,totaljerkface.com,unity3d.com,vg247.com,wowhead.com',
    'gaming' ],
  [ 'a10.com,absolutist.com,addictinggames.com,aeriagames.com,agame.com,alpha-wars.com,arcadeyum.com,armorgames.com,ballerarcade.com,battle.net,battlefield.com,bigfishgames.com,bioware.com,bitrhymes.com,candystand.com,conjurorthegame.com,crazymonkeygames.com,crusharcade.com,curse.com,cuttherope.net,dreammining.com,dressupgames.com,ea.com,easports.com,fps-pb.com,freearcade.com,freeonlinegames.com,friv.com,funplusgame.com,gamefly.com,gameforge.com,gamehouse.com,gamejolt.com,gameloft.com,gameoapp.com,gamepedia.com,gamersfirst.com,games.com,games.yahoo.com,gamesgames.com,gamezhero.com,gamingwonderland.com,ganymede.eu,goodgamestudios.com,gpotato.com,gsn.com,guildwars2.com,hirezstudios.com,igg.com,iwin.com,kahoot.it,king.com,kizi.com,kongregate.com,leagueoflegends.com,lolking.net,maxgames.com,minecraft-mp.com,minecraft.net,miniclip.com,mmo-play.com,mmorpg.com,mobafire.com,moviestarplanet.com,myonlinearcade.com,needforspeed.com,newgrounds.com,nexusmods.com,nintendo.com,noxxic.com,onrpg.com,origin.com,pch.com,peakgames.net,playstation.com,pogo.com,pokemon.com,popcap.com,primarygames.com,r2games.com,railnation.us,riotgames.com,roblox.com,rockstargames.com,runescape.com,shockwave.com,silvergames.com,spore.com,steamcommunity.com,steampowered.com,stickpage.com,swtor.com,tetrisfriends.com,thegamerstop.com,thesims3.com,twitch.tv,warthunder.com,wildtangent.com,worldoftanks.com,worldofwarcraft.com,worldofwarplanes.com,worldofwarships.com,xbox.com,y8.com,zone.msn.com,zynga.com,zyngawithfriends.com',
    'online gaming' ],
]);


const ALLOWED_LINK_SCHEMES = new Set(["http", "https"]);


const ALLOWED_IMAGE_SCHEMES = new Set(["https", "data"]);


const ALLOWED_URL_BASE = new Set(["mozilla.net", ""]);


const DIRECTORY_FRECENCY = 1000;


const SUGGESTED_FRECENCY = Infinity;


const FREQUENCY_CAP_FILE = "frequencyCap.json";


const DEFAULT_DAILY_FREQUENCY_CAP = 3;
const DEFAULT_TOTAL_FREQUENCY_CAP = 10;



const DEFAULT_PRUNE_TIME_DELTA = 10*24*60*60*1000;


const MIN_VISIBLE_HISTORY_TILES = 8;


const MAX_VISIBLE_HISTORY_TILES = 15;


const PING_SCORE_DIVISOR = 10000;


const PING_ACTIONS = ["block", "click", "pin", "sponsored", "sponsored_link", "unpin", "view"];


const INADJACENCY_SOURCE = "chrome://browser/content/newtab/newTab.inadjacent.json";






let DirectoryLinksProvider = {

  __linksURL: null,

  _observers: new Set(),

  
  _downloadDeferred: null,

  
  _downloadIntervalMS: 86400000,

  


  _enhancedLinks: new Map(),

  


  _suggestedLinks: new Map(),

  


  _frequencyCaps: {},

  


  _topSitesWithSuggestedLinks: new Set(),

  


  _inadjacentSites: new Set(),

  



  _avoidInadjacentSites: false,

  



  _newTabHasInadjacentSite: false,

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
        this.__linksURLModified = Services.prefs.prefHasUserValue(this._observedPrefs["linksURL"]);
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
      let enhanced = Services.prefs.getBoolPref(PREF_NEWTAB_ENHANCED);
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
    
    if (!link.frecent_sites) {
      return;
    }

    for (let suggestedSite of link.frecent_sites) {
      let suggestedMap = this._suggestedLinks.get(suggestedSite) || new Map();
      suggestedMap.set(link.url, link);
      this._setupStartEndTime(link);
      this._suggestedLinks.set(suggestedSite, suggestedMap);
    }
  },

  _fetchAndCacheLinks: function DirectoryLinksProvider_fetchAndCacheLinks(uri) {
    
    uri = uri.replace("%LOCALE%", this.locale);
    uri = uri.replace("%CHANNEL%", UpdateChannel.get());

    return this._downloadJsonData(uri).then(json => {
      return OS.File.writeAtomic(this._directoryFilePath, json, {tmpPath: this._directoryFilePath + ".tmp"});
    });
  },

  




  _downloadJsonData: function DirectoryLinksProvider__downloadJsonData(uri) {
    let deferred = Promise.defer();
    let xmlHttp = this._newXHR();

    xmlHttp.onload = function(aResponse) {
      let json = this.responseText;
      if (this.status && this.status != 200) {
        json = "{}";
      }
      deferred.resolve(json);
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

  


  _newXHR() {
    return new XMLHttpRequest({mozAnon: true});
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

  



  _setupStartEndTime: function DirectoryLinksProvider_setupStartEndTime(link) {
    
    
    
    
    
    
    if (!link.time_limits) {
      return;
    }

    let parsedTime;
    if (link.time_limits.start) {
      parsedTime = Date.parse(link.time_limits.start);
      if (parsedTime && !isNaN(parsedTime)) {
        link.startTime = parsedTime;
      }
    }
    if (link.time_limits.end) {
      parsedTime = Date.parse(link.time_limits.end);
      if (parsedTime && !isNaN(parsedTime)) {
        link.endTime = parsedTime;
      }
    }
  },

  


  _onCampaignTimeout: function DirectoryLinksProvider_onCampaignTimeout() {
    
    this._campaignTimeoutID = null;
    this._updateSuggestedTile();
  },

  


  _clearCampaignTimeout: function DirectoryLinksProvider_clearCampaignTimeout() {
    if (this._campaignTimeoutID) {
      clearTimeout(this._campaignTimeoutID);
      this._campaignTimeoutID = null;
    }
  },

  




  _setupCampaignTimeCheck: function DirectoryLinksProvider_setupCampaignTimeCheck(timeout) {
    
    if (!timeout || timeout <= 0) {
      return;
    }
    this._clearCampaignTimeout();
    
    this._campaignTimeoutID = setTimeout(this._onCampaignTimeout.bind(this), timeout);
  },

  






  _testLinkForCampaignTimeLimits: function DirectoryLinksProvider_testLinkForCampaignTimeLimits(link) {
    let currentTime = Date.now();
    
    if (link.startTime && link.startTime > currentTime) {
      
      return {use: false, timeoutDate: link.startTime};
    }
    
    if (link.endTime) {
      
      if (link.endTime <= currentTime) {
        return {use: false};
      }
      
      return {use: true, timeoutDate: link.endTime};
    }
    
    return {use: true};
  },

  






  reportSitesAction: function DirectoryLinksProvider_reportSitesAction(sites, action, triggeringSiteIndex) {
    
    if (action == "view") {
      sites.slice(0, triggeringSiteIndex + 1).forEach(site => {
        let {targetedSite, url} = site.link;
        if (targetedSite) {
          this._addFrequencyCapView(url);
        }
      });
    }
    
    
    
    
    else {
      
      let {frecent_sites, targetedSite, url} = sites[triggeringSiteIndex].link;
      if (frecent_sites || targetedSite) {
        this._setFrequencyCapClick(url);
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

    
    let ping = this._newXHR();
    ping.open("POST", pingEndPoint + (action == "view" ? "view" : "click"));
    ping.send(JSON.stringify(data));

    return Task.spawn(function* () {
      
      yield this._writeFrequencyCapFile();
      
      yield this._fetchAndCacheLinksIfNecessary();
    }.bind(this));
  },

  


  getEnhancedLink: function DirectoryLinksProvider_getEnhancedLink(link) {
    
    return link.enhancedImageURI && link ? link :
           this._enhancedLinks.get(NewTabUtils.extractSite(link.url));
  },

  



  getFrecentSitesName(sites) {
    return ALLOWED_FRECENT_SITES.get(sites.join(","));
  },

  






  isURLAllowed(url, allowed, checkBase) {
    
    if (!url) {
      return true;
    }

    let scheme = "", base = "";
    try {
      
      let uri = Services.io.newURI(url, null, null);
      scheme = uri.scheme;

      
      base = Services.eTLD.getBaseDomain(uri);
    }
    catch(ex) {}
    
    return allowed.has(scheme) && (!checkBase || ALLOWED_URL_BASE.has(base));
  },

  _escapeChars(text) {
    let charMap = {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&#039;'
    };

    return text.replace(/[&<>"']/g, (character) => charMap[character]);
  },

  



  getLinks: function DirectoryLinksProvider_getLinks(aCallback) {
    this._readDirectoryLinksFile().then(rawLinks => {
      
      this._enhancedLinks.clear();
      this._suggestedLinks.clear();
      this._clearCampaignTimeout();
      this._avoidInadjacentSites = false;

      
      let checkBase = !this.__linksURLModified;
      let validityFilter = function(link) {
        
        return this.isURLAllowed(link.url, ALLOWED_LINK_SCHEMES, false) &&
               this.isURLAllowed(link.imageURI, ALLOWED_IMAGE_SCHEMES, checkBase) &&
               this.isURLAllowed(link.enhancedImageURI, ALLOWED_IMAGE_SCHEMES, checkBase);
      }.bind(this);

      rawLinks.suggested.filter(validityFilter).forEach((link, position) => {
        
        let name = this.getFrecentSitesName(link.frecent_sites);
        if (name == undefined) {
          return;
        }

        let sanitizeFlags = ParserUtils.SanitizerCidEmbedsOnly |
          ParserUtils.SanitizerDropForms |
          ParserUtils.SanitizerDropNonCSSPresentation;

        link.explanation = this._escapeChars(link.explanation ? ParserUtils.convertToPlainText(link.explanation, sanitizeFlags, 0) : "");
        link.targetedName = this._escapeChars(ParserUtils.convertToPlainText(link.adgroup_name, sanitizeFlags, 0) || name);
        link.lastVisitDate = rawLinks.suggested.length - position;
        
        if (link.check_inadjacency) {
          this._avoidInadjacentSites = true;
        }

        
        
        this._cacheSuggestedLinks(link);
        this._updateFrequencyCapSettings(link);
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

      
      this._pruneFrequencyCapUrls();
      
      this._writeFrequencyCapFile();

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

    
    this._frequencyCapFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, FREQUENCY_CAP_FILE);
    
    this._inadjacentSitesUrl = INADJACENCY_SOURCE;

    NewTabUtils.placesProvider.addObserver(this);
    NewTabUtils.links.addObserver(this);

    return Task.spawn(function() {
      
      let doesFileExists = yield OS.File.exists(this._directoryFilePath);
      if (doesFileExists) {
        let fileInfo = yield OS.File.stat(this._directoryFilePath);
        this._lastDownloadMS = Date.parse(fileInfo.lastModificationDate);
      }
      
      yield this._readFrequencyCapFile();
      
      yield this._fetchAndCacheLinksIfNecessary();
      
      yield this._loadInadjacentSites();
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

    
    
    if (this._avoidInadjacentSites && this._isInadjacentLink(aLink)) {
      return true;
    }

    return false;
  },

  _populatePlacesLinks: function () {
    NewTabUtils.links.populateProviderCache(NewTabUtils.placesProvider, () => {
      this._handleManyLinksChanged();
    });
  },

  onDeleteURI: function(aProvider, aLink) {
    let {url} = aLink;
    
    
    this._removeTileClick(url).then(() => {
      this._callObservers("onDeleteURI", url);
    });
  },

  onClearHistory: function() {
    
    this._removeAllTileClicks().then(() => {
      this._callObservers("onClearHistory");
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

  _getCurrentTopSiteCount: function() {
    let visibleTopSiteCount = 0;
    let newTabLinks = NewTabUtils.links.getLinks();
    for (let link of newTabLinks.slice(0, MIN_VISIBLE_HISTORY_TILES)) {
      
      if (link && (link.type == "history" || link.type == "enhanced")) {
        visibleTopSiteCount++;
      }
    }
    
    
    this._newTabHasInadjacentSite = this._avoidInadjacentSites && this._checkForInadjacentSites(newTabLinks);

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

    
    
    
    
    
    let nextTimeout;
    let possibleLinks = new Map();
    let targetedSites = new Map();
    this._topSitesWithSuggestedLinks.forEach(topSiteWithSuggestedLink => {
      let suggestedLinksMap = this._suggestedLinks.get(topSiteWithSuggestedLink);
      suggestedLinksMap.forEach((suggestedLink, url) => {
        
        if (!this._testFrequencyCapLimits(url)) {
          return;
        }

        
        
        let {use, timeoutDate} = this._testLinkForCampaignTimeLimits(suggestedLink);
        
        if (timeoutDate && (!nextTimeout || nextTimeout > timeoutDate)) {
          nextTimeout = timeoutDate;
        }
        
        if (!use) {
          return;
        }

        
        if (suggestedLink.check_inadjacency && this._newTabHasInadjacentSite) {
          return;
        }

        possibleLinks.set(url, suggestedLink);

        
        
        if (!targetedSites.get(url)) {
          targetedSites.set(url, []);
        }
        targetedSites.get(url).push(topSiteWithSuggestedLink);
      })
    });

    
    if (nextTimeout) {
      this._setupCampaignTimeCheck(nextTimeout - Date.now());
    }

    
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

  



  _loadInadjacentSites: function DirectoryLinksProvider_loadInadjacentSites() {
    return this._downloadJsonData(this._inadjacentSitesUrl).then(jsonString => {
      let jsonObject = {};
      try {
        jsonObject = JSON.parse(jsonString);
      }
      catch (e) {
        Cu.reportError(e);
      }

      this._inadjacentSites = new Set(jsonObject.domains);
    });
  },

  




  _generateHash: function DirectoryLinksProvider_generateHash(value) {
    let byteArr = gUnicodeConverter.convertToByteArray(value);
    gCryptoHash.init(gCryptoHash.MD5);
    gCryptoHash.update(byteArr, byteArr.length);
    return gCryptoHash.finish(true);
  },

  




  _isInadjacentLink: function DirectoryLinksProvider_isInadjacentLink(link) {
    let baseDomain = link.baseDomain || NewTabUtils.extractSite(link.url || "");
    if (!baseDomain) {
        return false;
    }
    
    return this._inadjacentSites.has(this._generateHash(baseDomain));
  },

  




  _checkForInadjacentSites: function DirectoryLinksProvider_checkForInadjacentSites(newTabLink) {
    let links = newTabLink || NewTabUtils.links.getLinks();
    for (let link of links.slice(0, MAX_VISIBLE_HISTORY_TILES)) {
      
      if (this._isInadjacentLink(link)) {
        return true;
      }
    }
    return false;
  },

  





  _readJsonFile: Task.async(function* (filePath, nullObject) {
    let jsonObj;
    try {
      let binaryData = yield OS.File.read(filePath);
      let json = gTextDecoder.decode(binaryData);
      jsonObj = JSON.parse(json);
    }
    catch (e) {}
    return jsonObj || nullObject;
  }),

  




  _readFrequencyCapFile: Task.async(function* () {
    
    this._frequencyCaps = yield this._readJsonFile(this._frequencyCapFilePath, {});
  }),

  



  _writeFrequencyCapFile: function DirectoryLinksProvider_writeFrequencyCapFile() {
    let json = JSON.stringify(this._frequencyCaps || {});
    return OS.File.writeAtomic(this._frequencyCapFilePath, json, {tmpPath: this._frequencyCapFilePath + ".tmp"});
  },

  



  _clearFrequencyCap: function DirectoryLinksProvider_clearFrequencyCap() {
    this._frequencyCaps = {};
    return this._writeFrequencyCapFile();
  },

  


  _updateFrequencyCapSettings: function DirectoryLinksProvider_updateFrequencyCapSettings(link) {
    let capsObject = this._frequencyCaps[link.url];
    if (!capsObject) {
      
      capsObject = {
        dailyViews: 0,
        totalViews: 0,
        lastShownDate: 0,
      };
      this._frequencyCaps[link.url] = capsObject;
    }
    
    capsObject.lastUpdated = Date.now();
    
    if (link.frequency_caps) {
      capsObject.dailyCap = link.frequency_caps.daily || DEFAULT_DAILY_FREQUENCY_CAP;
      capsObject.totalCap = link.frequency_caps.total || DEFAULT_TOTAL_FREQUENCY_CAP;
    }
    else {
      
      capsObject.dailyCap = DEFAULT_DAILY_FREQUENCY_CAP;
      capsObject.totalCap = DEFAULT_TOTAL_FREQUENCY_CAP;
    }
  },

  






  _pruneFrequencyCapUrls: function DirectoryLinksProvider_pruneFrequencyCapUrls(timeDelta = DEFAULT_PRUNE_TIME_DELTA) {
    let timeThreshold = Date.now() - timeDelta;
    Object.keys(this._frequencyCaps).forEach(url => {
      if (this._frequencyCaps[url].lastUpdated <= timeThreshold) {
        delete this._frequencyCaps[url];
      }
    });
  },

  




  _wasToday: function DirectoryLinksProvider_wasToday(timestamp) {
    let showOn = new Date(timestamp);
    let today = new Date();
    
    return showOn.getDate() == today.getDate() &&
           showOn.getMonth() == today.getMonth() &&
           showOn.getYear() == today.getYear();
  },

  



  _addFrequencyCapView: function DirectoryLinksProvider_addFrequencyCapView(url) {
    let capObject = this._frequencyCaps[url];
    
    if (!capObject) {
      return;
    }

    
    if (!this._wasToday(capObject.lastShownDate)) {
      capObject.dailyViews = 0;
      
      capObject.lastShownDate = Date.now();
    }

    
    capObject.totalViews++;
    capObject.dailyViews++;

    
    if (capObject.totalViews >= capObject.totalCap ||
        capObject.dailyViews >= capObject.dailyCap) {
      this._updateSuggestedTile();
    }
  },

  



  _setFrequencyCapClick: function DirectoryLinksProvider_reportFrequencyCapClick(url) {
    let capObject = this._frequencyCaps[url];
    
    if (!capObject) {
      return;
    }
    capObject.clicked = true;
    
    this._updateSuggestedTile();
  },

  




  _testFrequencyCapLimits: function DirectoryLinksProvider_testFrequencyCapLimits(url) {
    let capObject = this._frequencyCaps[url];
    
    if (!capObject) {
      return false;
    }

    
    if (capObject.clicked || capObject.totalViews >= capObject.totalCap) {
      return false;
    }

    
    if (this._wasToday(capObject.lastShownDate) &&
        capObject.dailyViews >= capObject.dailyCap) {
      return false;
    }

    
    return true;
  },

  




  _removeTileClick: function DirectoryLinksProvider_removeTileClick(url = "") {
    
    let noTrailingSlashUrl = url.replace(/\/$/,"");
    let capObject = this._frequencyCaps[url] || this._frequencyCaps[noTrailingSlashUrl];
    
    if (!capObject) {
      return Promise.resolve();
    }
    
    delete capObject.clicked;
    return this._writeFrequencyCapFile();
  },

  



  _removeAllTileClicks: function DirectoryLinksProvider_removeAllTileClicks() {
    Object.keys(this._frequencyCaps).forEach(url => {
      delete this._frequencyCaps[url].clicked;
    });
    return this._writeFrequencyCapFile();
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
