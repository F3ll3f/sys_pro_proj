Ελευθέριος Μπόλκας 1115201900353

Περιεχόμενα:
Ένα αρχείο κειμένου inputFile, το README.txt, το script create_infiles.sh, το Makefile, τα αρχεία πηγιαίου κώδικα:
travelMonitor.cpp: Περιέχει τη main για το travelMonitor 
Monitor.cpp: Περιέχει τη main για το Monitor 
και οι φάκελοι:
AppTravelMonitor: Περιέχει το AppTravelMonitor.cpp με το header του στα οποία ορίζεται μια αντίστοιχη κλάση που υλοποιεί όλη την κυρίως λειτουργία του travelMonitor.
AppMonitor: Περιέχει το AppMonitor.cpp με το header του στα οποία ορίζεται μια αντίστοιχη κλάση που υλοποιεί όλη την κυρίως λειτουργία του Monitor.
MessageExchange: Περιέχει το messageExchange.cpp με το header του που υλοποιεί συναρτήσεις για ανταλλαγή μηνυμάτων μέσω ενός named pipe και με χρήση του buffer
BloomFilter: Περιέχει το bloomfilter.cpp με το header του που υλοποιεί το BloomFilter
Date_citizen: Περιέχει το date_citizen.cpp με το header του που ορίζει κλάσεις για date και citizen
HelpfulFunctions: Περιέχει το helpfulStringFunctions.cpp με το header του που περιέχει βοηθητικές συναρτήσεις για strings
OtherLists: Περιέχει τα namesList.cpp, statList.cpp, virusList.cpp με τα header τους που υλοποιούν διάφορων ειδών λίστες
SkipList: Περιέχει το skiplist.cpp με το header του που υλοποιεί την SkipList

Compilation της εφαρμογής:
Από το κεντρικό directory στο οποίο βρίσκεται και το Makefile εκτελούμε την make. Γίνεται separate compilation και στο τέλος μέσω του linker δημιουργούνται τα εκτελέσιμα travelMonitor και Monitor.

Εκτέλεση προγράμματος:
./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir

Εκτέλεση του Bash script:
./create_infiles.sh inputFile input_dir numFilesPerDirectory

Η εφαρμογή:
Πρωτόκολλα ανταλλαγής μηνυμάτων μέσω named pipes:
Με εξαίρεση το πρώτο μήνυμα από το parent process στα children που είναι η αποστολή του bufferSize(αυτό εξηγείται παρακάτω), όλα τα μηνύματα μεταξύ Parent 
process και Monitors ανταλλάσσονται με τον τρόπο που περιγράφεται εδώ. Για την ανταλλαγή των μηνυμάτων χρησιμοποιούνται οι συναρτήσεις που βρίσκονται στα 
αρχεία messageExchange.cpp, messageExchange.h. Εκεί ορίζονται η συνάρτηση GetMessage για την παραλαβή ενός μηνύματος και η συνάρτηση SendMessage για την 
αποστολή ενός μηνύματος. Και οι δύο συναρτήσεις στα ορίσματά τους παίρνουν ένα buffer με το bufferSize που χρησιμοποιείται για να γραφτεί και να διαβαστεί ένα 
μήνυμα(το buffer έχει φτιαχτεί στο travelMonitor και στο Monitor). Επίσης, παίρνουν στα ορίσματα έναν file descriptor που είναι το named pipe από το οποίο 
διαβάζεται και στέλνεται το μήνυμα αντίστοιχα. Κάθε μήνυμα που στέλνεται και διαβάζεται είναι τύπου δείκτης σε char. Η ερμηνεία του μηνύματος και η συνολική 
επικοινωνία μεταξύ parent και child process μπορεί να χρησιμοποιούν επιπλέον πρωτόκολλα(τα οποία περιγράφονται παρακάτω στην ανάλυση των travelMonitor και 
Monitor), ωστόσο πάντα η αποστολή ενός μηνύματος και η λήψη του μηνύματος ακολουθεί το πρωτόκολλο που αναλύεται εδώ και υλοποιείται από αυτές τις δύο 
συναρτήσεις. Για την αποστολή του μηνύματος, προστίθενται στην αρχή του 10 επιπλέον bytes που υποδεικνύουν το μέγεθος του αρχικού μηνύματος. Συγκεκριμένα κάθε 
byte είναι ένας αριθμός τύπου char που αναπαριστά ένα ψηφίο του μέγεθους του αρχικού μηνύματος. Το νέο μήνυμα που δημιουργείται χωρίζεται σε τμήματα έτσι ώστε 
κάθε τμήμα να μπορεί να χωρέσει στο buffer και ένα-ένα τα τμήματα αντιγράφονται στο buffer και γράφονται στο named pipe. Για τη λήψη του μηνύματος, αρχικά 
διαβάζονται τα πρώτα 10 bytes(σταδιακά στην περίπτωση που το buffer είναι μικρό), αποκωδικοποιείται το μέγεθος του υπόλοιπου μηνύματος και γίνεται allocate στο 
heap μνήμη όση το μέγεθος του μηνύματος. Έπειτα διαβάζονται από το named pipe το πολύ bufferSize bytes κάθε φορά στο buffer και αυτά αντιγραφόνται με τη σειρά 
στη μνήμη που κάναμε allocate. Όταν διαβαστεί όλο το μήνυμα, επιστρέφεται(μέσω της μεταβλητής Message των ορισμάτων της GetMessage, που είναι reference σε 
δείκτη σε char) αυτή η ένωση όλων των τμημάτων που διαβάστηκαν και αποτελεί το αρχικό μήνυμα που στάλθηκε. Επισημαίνεται ότι κατά τη διάρκεια τόσο της read όσο 
και της write αναγνωρίζεται ο πραγματικός αριθμός των bytes που διαβάστηκαν ή γράφτηκαν, έτσι ώστε αν δεν διαβάστηκε/γράφτηκε ο αναμενόμενος αριθμός από bytes 
λόγω interruption από κάποιο σήμα, η διαδικασία να συνεχίζεται από εκεί που είχε μείνει χωρίς πρόβλημα.

Αποκλειστικά για την αποστολή του πρώτου μηνύματος(bufferSize) από το parent process στα Monitors και μόνο, ακολουθείται το εξής πρωτόκολλο. Το bufferSize 
μετατρέπεται σε string(c string), αντιγράφεται στο buffer και στέλνεται μέσω του named pipe(εκτός αν bufferSize=1, όπου δεν χωράει στο buffer και στέλνεται σε 
δύο βήματα). Το monitor διαβάζει ένα-ένα τα bytes από το named pipe μέχρι να συναντήσει το χαρακτήρα '\0', όπου και σταματάει. Έτσι, το monitor μαθαίνει το 
bufferSize και μπορεί πλέον να δημιουργήσει το buffer για την επικοινωνία με το parent process.

Γενικές πληροφορίες για κάποιες κλάσεις:
Οι κλάσεις BloomFilter, SkipList, date, citizen, VirusList είναι αντίστοιχες με αυτές που υλοποιήθηκαν στην πρώτη εργασία και αναπαριστούν τα αντίστοιχα 
αντικείμενα. Ιδιαίτερα, επισημαίνεται ότι η VirusList αναπαριστά μια απλά συνδεδεμένη λίστα από ιούς όπου κάθε ιός συνοδεύεται με ένα δείκτη σε ένα 
BloomFilter, ένα δείκτη σε μία SkipList με τους vaccinated persons του ιού και ένα δείκτη σε μία SkipList με τους not vaccinated persons του ιού. Οι δείκτες 
στις SkipLists μπορεί να είναι NULL στην περίπτωση που δεν χρειαζόμαστε SkipLists, όπως στην περίπτωση του parent process.  Η κλάση NamesList αναπαριστά επίσης 
μία απλά συνδεδεμένη λίστα από strings.

Η κλάση ListOfLists που υπάρχει στο αρχείο statList.h υλοποιεί μία λίστα που χρησιμεύει για τη δημιουργία της κλάσης StatList του ίδιου αρχείου. Η τελευταία 
αποθηκεύει τις πληροφορίες που χρειαζόμαστε για όλα τα requests που έρχονται στο parent process ώστε να μπορούν υπολογιστούν κατάλληλα στατιστικά. Πιο 
συγκεκριμένα, η ListOfLists είναι μία απλά συνδεδεμένη λίστα όπου κάθε κόμβος της πέρα από τον δείκτη στον επόμενο κόμβο, έχει έναν δείκτη σε ένα string, μία 
ημερομηνία date(έχει την ημερομηνία 0-0-0 όταν δεν την χρειαζόμαστε) και ένα δείκτη σε μία ListOfLists ο οποιός μπορεί να είναι και NULL. Η StatList έχει ως 
μέλη της δύο NamesList(Μία για τους ιούς  και μία για τις χώρες που περιέχονται στα requests. Με αυτές αποφεύγουμε το data duplication) και μία ListOfLists που 
αποθηκεύει τα requests με τον εξής τρόπο. Αυτή η ListOfLists είναι μία λίστα με τα ονόματα των ιών που εμφανίζονται στα requests. Κάθε κόμβος της που περιέχει 
έναν ιό, περιέχει και ένα δείκτη σε μία ListOfLists όπου ο κάθε κόμβος της τελευταίας περιέχει το όνομα μιας χώρας. Κάθε τέτοιος κόμβος με το όνομα μιας χώρας 
συνοδεύεται επίσης και από ένα δείκτη σε μία ListOfLists που αποθηκεύει τα requests γι' αυτόν τον ιό και γι' αυτή τη χώρα. Ο κάθε κόμβος αυτής της τελευταίας 
ListOfLists συγκεκριμένα περιέχει την ημερομηνία για το ταξίδι του request και το string "Y" αν το request έγινε accepted ή το string "N" αν έγινε rejected. Με 
τις διάφορες μεθόδους της StatList προστίθενται τα requests και υπολογίζονται τα επιθυμητά στατιστικά.

Ανάλυση της υλοποίησης του travelMonitor:
Αφού γίνουν οι απαραίτητοι έλεγχοι, στην main(travelMonitor.cpp) δημιουργείται ένα αντικείμενο της κλάσης AppTravelMonitor στην οποία υλοποιούνται ουσιαστικά 
όλες οι λειτουργίες της εφαρμογής. Η κλάση AppTravelMonitor έχει μεταξύ άλλων στα πεδία της ένα πίνακα για τους file descriptors των named pipes από τα οποία 
διαβάζει μηνύματα, ένα πίνακα για τους file descriptors των named pipes στα οποία γράφει μηνύματα, ένα πίνακα για τα pids των Monitors και το buffer(char *) 
που χρησιμοποιείται για την ανταλλαγή μυνημάτων από την πλευρά του travelMonitor και δημιουργείται στον constructor. Επίσης, έχει ένα 
πίνακα(BloomFiltersOfMonitors) με ένα δείκτη για κάθε monitor σε μία VirusList που περιέχει μια λίστα από τα BloomFilters που θα λάβει από τα monitors. Ένα 
πεδίο της είναι, ακόμα, ένας πίνακας(CountriesOfMonitors) που περιέχει ένα δείκτη για κάθε monitor σε ένα πίνακα με τα ονόματα των χωρών που αναλαμβάνει αυτό 
το monitor και τα οποία καθορίζονται από τη μοιρασιά τους round-robin που γίνεται επίσης στον constructor. Επιπλέον, υπάρχει και μια δομή τύπου StatList στην 
οποία αποθηκεύονται τα requests που λαμβάνει το parent process.

Στον constructor της AppTravelMonitor, δημιουργούνται 2 named pipes για κάθε monitor, ένα για λήψη μηνυμάτων και ένα για αποστολή μηνυμάτων(αν κατά τη 
δημιουργία τους προκύψει error επειδή υπάρχουν ήδη από προηγούμενη εκτέλεση της εφαρμογής, χρησιμοποιούνται αυτά). Έπειτα με χρήση fork και exec δημιουργούνται 
τα monitors(αναλύονται παρακάτω). Το parent process αποθηκεύει τα pids των monitors και ανοίγει τα μισά named pipes με σκοπό να διαβάζει από εκεί(στο εξήs θα 
λέγονται receiver named pipes), και τα άλλα μισά για να γράφει σε αυτά(στο εξήs θα λέγονται sender named pipes). Δεν χρησιμοποιείται το O_NONBLOCK, με σκοπό να 
συγχρονίζονται τα monitors με το parent process. Στη συνέχεια αρχικοποιούνται τα διάφορα πεδία της κλάσης AppTravelMonitor και παράλληλα στέλνονται στα 
Monitors οι πληροφορίες που χρειάζονται. Το πρώτο μήνυμα(bufferSize) σε κάθε monitor στέλνεται με το πρωτόκολλο που περιγράφτηκε παραπάνω, ενώ όλα τα άλλα 
μηνύματα στέλνονται με τη χρήση των συναρτήσεων GetMessage και SendMessage(που επίσης περιγράφηκαν παραπάνω) οι οποίες καλούνται με κατάλληλα ορίσματα από τις 
αντίστοιχες μεθόδους ReceiveMessageFromMonitor, SendMessageToMonitor της κλάσης.  

Έπειτα(από τον constructor ακόμα) στέλνονται σε κάθε monitor κατά σειρά το όνομα του input_dir, ο αριθμός των directories που αναλαμβάνει, τα ονόματα των 
directories και το sizeOfBloom. Όλα τα παραπάνω μετατρέπονται σε c-strings για την αποστολή καθώς τα μηνύματα πρέπει να είναι τύπου char *, όπως περιέγραψα στο 
πρωτόκολλο. Στη συνέχεια, το travelMonitor περιμένει να λάβει τα BloomFilters από κάθε monitor. Για να μην περιμένει όσα monitors δεν είναι έτοιμα, μέσω της 
select παρακολουθούνται τα receiver named pipes και όταν κάποιο βρεθεί ότι έχει δεδομένα για διάβασμα, διαβάζει από εκείνο το μήνυμα. Κάθε BloomFilter 
στέλνεται/λαμβάνεται ως ένα μήνυμα char * που αποτελεί το array του BloomFilter(ο πίνακας των bytes του δηλαδή με τον τρόπο που έχει υλοποιηθεί). Όταν λάβει 
όλα τα BloomFilter από ένα monitor, το parent process ενημερώνει τις δομές του και λαμβάνει ακόμα ένα μήνυμα(το string "READY") που επιβεβαιώνει ότι το 
αντίστοιχο monitor έχει κάνει όλες τις ενέργειες που χρειάζεται και είναι έτοιμο. Όταν ληφθούν όλα τα "READY", τελειώνει ο constructor και καλείται από τη main 
στο αντικείμενο αυτής της κλάσης η μέθοδος StartTravelMonitor για την κυρίως λειτουργία του. Επισημαίνεται ότι η σειρά αυτή των μηνυμάτων ακολουθείται και από 
το parent και από τα children processes, καθώς λόγω της γνώση αυτής της σειράς ερμηνεύονται σωστά τα μηνύματα.

Στην αρχή της StartTravelMonitor, ρυθμίζεται ο χειρισμός των σημάτων SIGCHLD, SIGINT, SIGQUIT. Αν έρθει το σήμα SIGCHLD, το static μέλος πεδίο Mode της 
AppTravelMonitor παίρνει τιμή 1, αν έρθει SIGINT ή SIGQUIT η τιμή του γίνεται 2, ενώ αν έρθει SIGINT ή SIGQUIT μετά από SIGCHLD, χωρίς να έχει προλάβει να 
ξεκινήσει η λειτουργία του τελευταίου, παίρνει τιμή 3. Έπειτα επαναλαμβάνεται συνεχώς η εξής διαδικασία. Ελέγχεται πρώτα αν το Mode έχει τιμή διάφορη του 0(που 
σημαίνει ότι δεν έχει έρθει κάποιο σήμα). Αν ναι, εκτελείται η αντίστοιχη λειτουργία, αλλιώς καλείται η select που έχει ρυθμιστεί να παρακολουθεί το stdin και 
τα receiver named pipes. H select μπλοκάρει και αν επιστρέψει ελέγχουμε την τιμή που επέστρεψε. Αν επέστρεψε -1(και errno==EINTR), σημαίνει ότι ήρθε κάποιο 
σήμα οπότε ελέγχουμε το Mode και εκτελούμε την αντίστοιχη λειτουργία. Αλλιώς, σημαίνει είτε ότι stdin έχει δεδομένα, οπότε ο χρήστης έγραψε μια εντολή, την 
οποία ελέγχουμε και εκτελούμε είτε ότι κάποιο receiver named pipe έχει δεδομένα για διάβασμα. Στην τελευταία περίπτωση σημαίνει απαραίτητα ότι κάποιο Monitor 
έλαβε SIGUSR1 και έχει στείλει τα ενημερωμένα BloomFilters του, οπότε τα διαβάζουμε όπως παραπάνω και ενημερώνουμε τα αντίστοιχα του parent process.

Όταν αναγνωριστεί κάποια εντολή, εκτελείται η αντίστοιχη μέθοδος που πραγματοποιεί την λειτουργία που αναφέρει η εκφώνηση. Ιδιαίτερα, στις εντολές 
/travelRequest και /searchVaccinationStatus που χρειάζεται πιθανόν η αποστολή δεδομένων μέσω named pipe, ακολουθούνται επιπλέον τα εξής πρωτόκολλα. Στην 
/travelRequest, αν χρειαστεί, στέλνεται από το sender named pipe το όνομα της εντολής και μετά ένα ένα τα ορίσματα της ως c strings στο κατάλληλο monitor. 
Έπειτα λαμβάνεται η απάντηση ("NO", "YES "+date ή "X" στην περίπτωση inconsistent data) και εκτελείται η υπόλοιπη λειτουργία. Στην περίπτωση της 
/searchVaccinationStatus στέλνεται όμοια πρώτα ως μήνυμα η εντολή και μετά ως string το citizenID. Όποια monitor εντοπίσουν το citizenID στα αρχεία τους(πάνω 
από ένα μόνο στην περίπτωση που το inputFile έχει inconsistent data) στέλνουν ως απάντηση το string "Y", ενώ τα υπόλοιπα το "N". Το travelMonitor επιλέγει ένα 
από αυτά που έστειλαν "Y", αν υπήρξε κάποιο, και του στέλνει το string "A" για αποδοχή επικοινωνίας, στέλνωντας στα υπόλοιπα το "R". Από το monitor που 
επέλεξε, λαμβάνει τα στοιχεία του πολίτη με τον τρόπο που σκοπεούμε να τα τυπώσουμε(ένα μήνυμα για κάθε γραμμή, πάντα με την αναπαράσταση ως c string), 
λαμβάνει τον αριθμό των εμβολιασμών του πολίτη και τον αριθμό των αρνητικών records για εμβολιασμούς. Τελικά, λαμβάνει ένα-ένα τα θετικά records και μετά τα 
αρνητικά, με τον τρόπο που θα τυπωθούν(ένα μήνυμα για κάθε record). Σημειώνεται, τέλος, ότι στην εντολή /addVaccinationRecords, στέλνεται το SIGUSR1 στο 
κατάλληλο monitor και μετά το parent process περιμένει να λάβει τα BloomFilters λειτουργώντας όπως στην αντίστοιχη περίπτωση παραπάνω. 

Ανάλυση της υλοποίησης του Monitor:
Στην main(Monitor.cpp) ανοίγουν τα δύο named pipe, το ένα για ανάγνωση και το άλλο για γράψιμο, (χωρίς O_NONBLOCK κι εδώ, για να υπάρχει συγχρονισμός) που 
βρίσκονται στα ορίσματα και δημιουργείται ένα αντικείμενο της κλάσης AppMonitor στην οποία υλοποιείται όλη η λειτουργία του Monitor. Η κλάση αυτή έχει μεταξύ 
άλλων στα πεδία της τους file decriptors των named pipes, τα bufferSize, sizeOfBloom, input_dir , το buffer που δημιουργείται στον constructor και 
χρησιμοποιείται για την ανταλλαγή μυνημάτων από την πλευρά του Monitor, ένα πίνακα Countries με τις χώρες που διαχειρίζεται και ένα πίνακα FileNames που 
περιέχει για κάθε χώρα, ένα δείκτη σε μία λίστα με τα ονόματα των αρχείων της χώρας που έχει μέχρι τώρα διαβάσει. Επίσης, έχει ένα πεδίο που είναι δείκτης σε 
SkipList στην οποία αποθηκεύονται όλοι οι πολίτες που εντοπίζονται στα records και ένα πεδίο που είναι δείκτης σε μία VirusList και περιλαμβάνει για κάθε ιό 
που εντοπίζεται, ένα BloomFilter και 2 SkipLists(vaccinated and not-vaccinated) που παίρνουν επίσης δεδομένα από τα records των αρχείων. Τέλος, υπάρχουν και 
δύο counters που μετράνε τα accepted και rejected requests που λήφθηκαν.

Στον constructor της AppMonitor λαμβάνεται αρχικά το bufferSize με το πρωτόκολλο για το πρώτο μήνυμα που έχει περιγραφεί, ενώ τα υπόλοιπα μηνύματα λαμβάνονται 
και στέλνονται μέσω των μεθόδων ReceiveMessageFromParent, SendMessageToParent που χρησιμοποιούν τις συναρτήσεις(που επίσης περιέγραψα παραπάνω) GetMessage και 
SendMessage με κατάλληλα ορίσματα. Αφού ληφθεί το bufferSize και δημιουργηθεί το buffer, αρχικοποιούνται όλες οι απαραίτητες δομές και λαμβάνονται παράλληλα 
από το αντίστοιχο named pipe τα μηνύματα που στάλθηκαν από το parent process με τη σειρά που αναλύθηκε παραπάνω. Έπειτα διαβάζονται τα αρχεία, ενημερώνονται οι 
δομές και στέλνονται τα BloomFilters στο parent process. Τότε, ο constructor τελειώνει, και καλείται η StartMonitor μέθοδος η οποία υλοποιεί την κυρίως 
λειτουργία του Monitor. 

Στην αρχή της StartMonitor, ρυθμίζεται ο χειρισμός των σημάτων SIGUSR1, SIGINT, SIGQUIT. Αν έρθει το σήμα SIGINT ή SIGQUIT, το static μέλος πεδίο Mode της 
AppMonitor παίρνει τιμή 1, αν έρθει SIGUSR1 η τιμή του γίνεται 2, ενώ αν έρθει SIGUSR1 και τουλάχιστον ένα εκ των SIGINT ή SIGQUIT, χωρίς να έχει εκτελεστεί 
κάποια από τις δύο λειτουργίες, παίρνει τιμή 3(σε αυτή την περίπτωση εκτελείται πρώτα η λειτουργία της SIGUSR1). Έπειτα στέλνεται στο parent process το string 
"READY" που δείχνει ότι το monitor είναι έτοιμο, και εισέρχεται στην εξής επαναληπτική διαδικασία. Ελέγχει την τιμή του Mode να δει αν έχει ληφθεί κάποιο σήμα 
και αν έχει μη μηδενική τιμή(Mode=0: κανένα σήμα), εκτελεί την κατάλληλη λειτουργία. Αλλιώς καλεί τη select η οποία έχει ρυθμιστεί να παρακολουθεί το named 
pipe από το οποίο λαμβάνονται μηνύματα. Η select τότε μπλοκάρει. Αν η select επιστρέψει -1 και errno==EINTR, σημαίνει ότι ήρθε κάποιο σήμα και ελέγχοντας το 
Mode, βρίσκουμε ποιο είναι και εκτελείται η αντίστοιχη λειτουργία. Αλλιώς, αν επιστρέψει άλλη τιμή, σημαίνει πως το fifo έχει δεδομένα για διάβασμα, οπότε 
διαβάζεται η εντολή που ήρθε από το parent process και εκτελείται η αντίστοιχη μέθοδος. Οι εντολές εκτελούνται όπως περιγράφεται στην εκφώνηση και ο τρόπος που 
ανταλλάσσονται τα μηνύματα με το travelMonitor στην κάθε εντολή αναλύθηκε στην αντίστοιχη περιγραφή του travelMonitor παραπάνω.

Το bash script:
Αρχικά, διαβάζονται όλες οι γραμμές του inputFile και οι χώρες που συναντώνται, αν δεν έχει δημιουργηθεί ήδη directory για αυτές, αποθηκεύονται σε ένα array, 
φτιάχνοντας ταυτόχρονα το αντίστοιχο directory και τα αρχεία(κενά αρχικά) του directory. Μετά για κάθε χώρα γίνεται το εξής. Βρίσκονται όλες οι εγγραφές αυτής 
της χώρας και εισάγονται σε ένα προσωρινό αρχείο. Μετά διαβάζουμε μία μία τις γραμμές αυτού του αρχείου και τις μοιράζουμε-προσθέτουμε round-robin σε κάποια 
strings, όπου κάθε string περιέχει όλες τις γραμμές που προορίζονται για ένα συγκεκριμένο αρχείο μιας χώρας. Τέλος, κάθε string  γράφεται στο αντίστοιχο αρχείο 
και γίνεται αυτό για όλες τις χώρες.


