/*
 * Medicine Reminder & Expiry Tracker
 *
 * Two modes, chosen when the program starts:
 *   1. Consumer   - tracks personal medicines, dose reminders, expiry, low stock
 *   2. Pharmacist - tracks stock (batch, supplier, price), expiry, low stock,
 *                   dispensing. No dose-timing questions at all.
 *
 * Each mode keeps its own data file so the two never mix.
 *
 * Compile:  gcc project.c -o project
 * Run:      .\project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MODE_CONSUMER    1
#define MODE_PHARMACIST  2

#define NAME_LEN         50
#define BATCH_LEN        20
#define SUPPLIER_LEN     40
#define TIME_LEN         8
#define FILE_LEN         40
#define EXPIRY_WARN_DAYS 30
#define INITIAL_CAPACITY 10

/* ---------- Data types ---------- */

typedef struct {
    int day, month, year;
} Date;

typedef struct {
    int  id;
    char name[NAME_LEN];
    int  quantity;
    int  minStock;          /* low-stock alert / reorder level            */
    Date expiry;

    /* consumer-only fields */
    int  dosesPerDay;
    char doseTime[TIME_LEN];    /* time of first dose, HH:MM              */
    int  dosesTakenToday;
    Date lastTaken;

    /* pharmacist-only fields */
    char  batch[BATCH_LEN];
    char  supplier[SUPPLIER_LEN];
    float price;
} Medicine;

typedef struct {
    Medicine *data;
    int  count;
    int  capacity;
    int  nextId;
    char filename[FILE_LEN];
} Store;

/* ---------- Input helpers ---------- */

void readLine(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput closed. Exiting.\n");
        exit(1);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int c;                                  /* discard the rest of a long line */
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
}

void readRequired(const char *prompt, char *buf, int size) {
    do {
        readLine(prompt, buf, size);
        if (buf[0] == '\0') printf("  This field cannot be empty.\n");
    } while (buf[0] == '\0');
}

int readInt(const char *prompt, int min, int max) {
    char line[64];
    int value;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%d", &value) == 1 && value >= min && value <= max)
            return value;
        printf("  Please enter a number between %d and %d.\n", min, max);
    }
}

float readFloat(const char *prompt, float min, float max) {
    char line[64];
    float value;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%f", &value) == 1 && value >= min && value <= max)
            return value;
        printf("  Please enter a number between %.2f and %.2f.\n", min, max);
    }
}

void waitForEnter(void) {
    char tmp[8];
    readLine("\nPress Enter to continue...", tmp, sizeof tmp);
}

int askYesNo(const char *prompt) {
    char ans[8];
    readLine(prompt, ans, sizeof ans);
    return tolower((unsigned char)ans[0]) == 'y';
}

/* ---------- Date helpers ---------- */

int isLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int isValidDate(Date d) {
    static const int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (d.year < 2000 || d.year > 2100) return 0;
    if (d.month < 1 || d.month > 12) return 0;
    int maxDay = daysInMonth[d.month - 1];
    if (d.month == 2 && isLeap(d.year)) maxDay = 29;
    return d.day >= 1 && d.day <= maxDay;
}

Date readDate(const char *prompt) {
    char line[64];
    Date d;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%d-%d-%d", &d.day, &d.month, &d.year) == 3 && isValidDate(d))
            return d;
        printf("  Invalid date. Use DD-MM-YYYY (example: 25-12-2026).\n");
    }
}

void readTime(const char *prompt, char *out) {
    char line[32];
    int h, m;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%d:%d", &h, &m) == 2 && h >= 0 && h <= 23 && m >= 0 && m <= 59) {
            snprintf(out, TIME_LEN, "%02d:%02d", h, m);
            return;
        }
        printf("  Use 24-hour HH:MM (example: 08:30).\n");
    }
}

Date getToday(void) {
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    Date d;
    d.day   = lt->tm_mday;
    d.month = lt->tm_mon + 1;
    d.year  = lt->tm_year + 1900;
    return d;
}

/* Converts a date to a running day count so dates can be subtracted. */
long toDayNumber(Date d) {
    int y = d.year, m = d.month;
    if (m <= 2) { y--; m += 12; }
    return 365L * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d.day;
}

long compareDates(Date a, Date b) {
    return toDayNumber(a) - toDayNumber(b);
}

long daysLeft(Date expiry, Date today) {
    return compareDates(expiry, today);
}

void formatDate(Date d, char *buf, int size) {
    snprintf(buf, size, "%02d-%02d-%04d", d.day, d.month, d.year);
}

const char *expiryStatus(Date expiry, Date today) {
    long left = daysLeft(expiry, today);
    if (left < 0) return "EXPIRED";
    if (left <= EXPIRY_WARN_DAYS) return "EXPIRING SOON";
    return "OK";
}

/* ---------- Store management and file handling ---------- */

void reserve(Store *s, int needed) {
    if (needed <= s->capacity) return;
    int newCap = s->capacity;
    while (newCap < needed) newCap *= 2;
    Medicine *p = realloc(s->data, (size_t)newCap * sizeof(Medicine));
    if (p == NULL) {
        printf("Out of memory.\n");
        exit(1);
    }
    s->data = p;
    s->capacity = newCap;
}

void initStore(Store *s, const char *filename) {
    s->data = malloc(INITIAL_CAPACITY * sizeof(Medicine));
    if (s->data == NULL) {
        printf("Out of memory.\n");
        exit(1);
    }
    s->count = 0;
    s->capacity = INITIAL_CAPACITY;
    s->nextId = 1;
    snprintf(s->filename, FILE_LEN, "%s", filename);
}

void saveStore(const Store *s) {
    FILE *fp = fopen(s->filename, "wb");
    if (fp == NULL) {
        printf("Error: could not save to '%s'.\n", s->filename);
        return;
    }
    fwrite(&s->count, sizeof(int), 1, fp);
    fwrite(&s->nextId, sizeof(int), 1, fp);
    fwrite(s->data, sizeof(Medicine), (size_t)s->count, fp);
    fclose(fp);
}

void loadStore(Store *s) {
    FILE *fp = fopen(s->filename, "rb");
    if (fp == NULL) return;                     /* first run: no file yet */

    int count, nextId;
    if (fread(&count, sizeof(int), 1, fp) != 1 ||
        fread(&nextId, sizeof(int), 1, fp) != 1 || count < 0) {
        printf("Warning: '%s' is unreadable. Starting with an empty list.\n", s->filename);
        fclose(fp);
        return;
    }
    reserve(s, count > 0 ? count : 1);
    size_t got = fread(s->data, sizeof(Medicine), (size_t)count, fp);
    if ((int)got != count)
        printf("Warning: '%s' looks damaged. Loaded %d of %d records.\n",
               s->filename, (int)got, count);
    s->count = (int)got;
    s->nextId = nextId;
    fclose(fp);
}

Medicine *findById(Store *s, int id) {
    for (int i = 0; i < s->count; i++)
        if (s->data[i].id == id) return &s->data[i];
    return NULL;
}

int containsIgnoreCase(const char *text, const char *pattern) {
    size_t tl = strlen(text), pl = strlen(pattern);
    if (pl == 0) return 1;
    for (size_t i = 0; i + pl <= tl; i++) {
        size_t j = 0;
        while (j < pl && tolower((unsigned char)text[i + j]) == tolower((unsigned char)pattern[j]))
            j++;
        if (j == pl) return 1;
    }
    return 0;
}

/* Doses taken only count for the day they were recorded. */
int dosesTakenOn(const Medicine *m, Date today) {
    return (compareDates(m->lastTaken, today) == 0) ? m->dosesTakenToday : 0;
}

/* ---------- Display ---------- */

void printHeader(int mode) {
    if (mode == MODE_CONSUMER) {
        printf("\n%-4s %-20s %-6s %-11s %-14s %-6s %-7s %s\n",
               "ID", "Name", "Qty", "Expiry", "Status", "Doses", "Time", "Taken");
        printf("--------------------------------------------------------------------------------\n");
    } else {
        printf("\n%-4s %-20s %-10s %-14s %-6s %-5s %-9s %-11s %s\n",
               "ID", "Name", "Batch", "Supplier", "Qty", "Min", "Price", "Expiry", "Status");
        printf("-------------------------------------------------------------------------------------------------\n");
    }
}

void printRow(const Medicine *m, int mode, Date today) {
    char dateStr[16];
    formatDate(m->expiry, dateStr, sizeof dateStr);

    if (mode == MODE_CONSUMER) {
        printf("%-4d %-20.20s %-6d %-11s %-14s %-6d %-7s %d/%d\n",
               m->id, m->name, m->quantity, dateStr,
               expiryStatus(m->expiry, today), m->dosesPerDay, m->doseTime,
               dosesTakenOn(m, today), m->dosesPerDay);
    } else {
        printf("%-4d %-20.20s %-10.10s %-14.14s %-6d %-5d %-9.2f %-11s %s\n",
               m->id, m->name, m->batch, m->supplier, m->quantity, m->minStock,
               m->price, dateStr, expiryStatus(m->expiry, today));
    }
}

void displayAll(const Store *s, int mode, Date today) {
    if (s->count == 0) {
        printf("\nNo medicines stored yet.\n");
        return;
    }
    printHeader(mode);
    for (int i = 0; i < s->count; i++)
        printRow(&s->data[i], mode, today);
    printf("Total: %d medicine(s)\n", s->count);
}

void showStartupAlerts(const Store *s, Date today) {
    int expired = 0, soon = 0, low = 0;
    for (int i = 0; i < s->count; i++) {
        long left = daysLeft(s->data[i].expiry, today);
        if (left < 0) expired++;
        else if (left <= EXPIRY_WARN_DAYS) soon++;
        if (s->data[i].quantity <= s->data[i].minStock) low++;
    }
    if (expired || soon || low)
        printf("\n*** ALERTS: %d expired, %d expiring within %d days, %d low on stock ***\n",
               expired, soon, EXPIRY_WARN_DAYS, low);
}

/* ---------- Menu ---------- */

int selectMode(void) {
    printf("=====================================\n");
    printf("   MEDICINE REMINDER & EXPIRY TRACKER\n");
    printf("=====================================\n");
    printf("Who is using this system?\n");
    printf("  1. Consumer   (track my own medicines and doses)\n");
    printf("  2. Pharmacist (manage stock, batches and expiry)\n");
    return readInt("Enter choice: ", 1, 2);
}

void showMenu(int mode) {
    if (mode == MODE_CONSUMER)
        printf("\n========== CONSUMER MENU ==========\n");
    else
        printf("\n========== PHARMACIST MENU ==========\n");

    printf(" 1. Add medicine\n");
    printf(" 2. View all medicines\n");
    printf(" 3. Search by name\n");
    printf(" 4. Update medicine\n");
    printf(" 5. Delete medicine\n");
    printf(" 6. Expiry report\n");
    printf(" 7. Low stock report\n");
    if (mode == MODE_CONSUMER) {
        printf(" 8. Take a dose\n");
        printf(" 9. Today's reminders\n");
    } else {
        printf(" 8. Dispense stock\n");
    }
    printf("10. Sort by expiry date\n");
    printf(" 0. Save and exit\n");
}

/* ---------- Features ---------- */

void addMedicine(Store *s, int mode, Date today) {
    reserve(s, s->count + 1);

    Medicine m;
    memset(&m, 0, sizeof m);
    m.id = s->nextId;

    readRequired("Medicine name: ", m.name, NAME_LEN);
    m.quantity = readInt("Quantity in stock: ", 0, 100000);
    m.expiry = readDate("Expiry date (DD-MM-YYYY): ");

    if (mode == MODE_CONSUMER) {
        m.minStock    = readInt("Warn me when stock falls to (units): ", 0, 100000);
        m.dosesPerDay = readInt("Doses per day (1-6): ", 1, 6);
        readTime("Time of first dose (HH:MM): ", m.doseTime);
    } else {
        m.minStock = readInt("Minimum stock level (reorder point): ", 0, 100000);
        readRequired("Batch number: ", m.batch, BATCH_LEN);
        readRequired("Supplier: ", m.supplier, SUPPLIER_LEN);
        m.price = readFloat("Price per unit: ", 0, 1000000);
    }
    m.lastTaken = today;

    s->data[s->count++] = m;
    s->nextId++;
    saveStore(s);
    printf("\nAdded '%s' with ID %d.\n", m.name, m.id);

    if (daysLeft(m.expiry, today) < 0)
        printf("WARNING: this medicine is already expired.\n");
}

void searchByName(const Store *s, int mode, Date today) {
    char query[NAME_LEN];
    readRequired("Search for: ", query, NAME_LEN);

    int found = 0;
    for (int i = 0; i < s->count; i++) {
        if (containsIgnoreCase(s->data[i].name, query)) {
            if (!found) printHeader(mode);
            printRow(&s->data[i], mode, today);
            found++;
        }
    }
    if (!found) printf("No medicine matching '%s'.\n", query);
    else printf("Found %d match(es).\n", found);
}

void updateMedicine(Store *s, int mode) {
    if (s->count == 0) {
        printf("No medicines stored yet.\n");
        return;
    }
    int id = readInt("Enter medicine ID to update: ", 1, 1000000);
    Medicine *m = findById(s, id);
    if (m == NULL) {
        printf("No medicine with ID %d.\n", id);
        return;
    }

    printf("\nUpdating '%s'\n", m->name);
    printf("  1. Name\n");
    printf("  2. Quantity\n");
    printf("  3. Expiry date\n");
    printf("  4. %s\n", mode == MODE_CONSUMER ? "Low stock warning level" : "Minimum stock level");
    if (mode == MODE_CONSUMER) {
        printf("  5. Doses per day\n");
        printf("  6. Time of first dose\n");
    } else {
        printf("  5. Batch number\n");
        printf("  6. Supplier\n");
        printf("  7. Price per unit\n");
    }
    int maxOpt = (mode == MODE_CONSUMER) ? 6 : 7;
    int opt = readInt("What do you want to change (0 to cancel)? ", 0, maxOpt);

    switch (opt) {
        case 0:
            printf("Cancelled.\n");
            return;
        case 1: readRequired("New name: ", m->name, NAME_LEN); break;
        case 2: m->quantity = readInt("New quantity: ", 0, 100000); break;
        case 3: m->expiry = readDate("New expiry date (DD-MM-YYYY): "); break;
        case 4: m->minStock = readInt("New level: ", 0, 100000); break;
        case 5:
            if (mode == MODE_CONSUMER)
                m->dosesPerDay = readInt("New doses per day (1-6): ", 1, 6);
            else
                readRequired("New batch number: ", m->batch, BATCH_LEN);
            break;
        case 6:
            if (mode == MODE_CONSUMER)
                readTime("New time of first dose (HH:MM): ", m->doseTime);
            else
                readRequired("New supplier: ", m->supplier, SUPPLIER_LEN);
            break;
        case 7:
            m->price = readFloat("New price per unit: ", 0, 1000000);
            break;
    }
    saveStore(s);
    printf("Medicine updated.\n");
}

void deleteMedicine(Store *s) {
    if (s->count == 0) {
        printf("No medicines stored yet.\n");
        return;
    }
    int id = readInt("Enter medicine ID to delete: ", 1, 1000000);
    Medicine *m = findById(s, id);
    if (m == NULL) {
        printf("No medicine with ID %d.\n", id);
        return;
    }

    char prompt[96];
    snprintf(prompt, sizeof prompt, "Delete '%s'? (y/n): ", m->name);
    if (!askYesNo(prompt)) {
        printf("Cancelled.\n");
        return;
    }

    int idx = (int)(m - s->data);
    for (int i = idx; i < s->count - 1; i++)
        s->data[i] = s->data[i + 1];
    s->count--;
    saveStore(s);
    printf("Deleted.\n");
}

void expiryReport(const Store *s, int mode, Date today) {
    printf("\n--- Expiry report (expired or within %d days) ---\n", EXPIRY_WARN_DAYS);
    int found = 0, expired = 0;
    for (int i = 0; i < s->count; i++) {
        long left = daysLeft(s->data[i].expiry, today);
        if (left <= EXPIRY_WARN_DAYS) {
            if (!found) printHeader(mode);
            printRow(&s->data[i], mode, today);
            found++;
            if (left < 0) expired++;
        }
    }
    if (!found) {
        printf("Nothing is expired or expiring soon.\n");
        return;
    }
    printf("%d item(s) need attention.\n", found);
    if (expired > 0) {
        if (mode == MODE_PHARMACIST)
            printf("Remove the %d expired item(s) from sale.\n", expired);
        else
            printf("Do not use the %d expired item(s). Dispose of them safely.\n", expired);
    }
}

void lowStockReport(const Store *s, int mode, Date today) {
    printf("\n--- Low stock report ---\n");
    int found = 0;
    for (int i = 0; i < s->count; i++) {
        if (s->data[i].quantity <= s->data[i].minStock) {
            if (!found) printHeader(mode);
            printRow(&s->data[i], mode, today);
            found++;
        }
    }
    if (!found) printf("All medicines are above their stock level.\n");
    else printf("%d item(s) are low on stock.\n", found);
}

/* Consumer: record one dose being taken. */
void takeDose(Store *s, Date today) {
    if (s->count == 0) {
        printf("No medicines stored yet.\n");
        return;
    }
    displayAll(s, MODE_CONSUMER, today);

    int id = readInt("\nEnter ID of the medicine you took (1+): ", 1, 1000000);
    Medicine *m = findById(s, id);
    if (m == NULL) {
        printf("No medicine with ID %d.\n", id);
        return;
    }
    if (daysLeft(m->expiry, today) < 0) {
        printf("'%s' has expired. Please do not use it; ask a pharmacist.\n", m->name);
        return;
    }
    if (m->quantity <= 0) {
        printf("'%s' is out of stock.\n", m->name);
        return;
    }

    int taken = dosesTakenOn(m, today);
    if (taken >= m->dosesPerDay) {
        printf("You have already recorded all %d dose(s) of '%s' today.\n",
               m->dosesPerDay, m->name);
        printf("Check your prescription before taking more.\n");
        return;
    }

    m->dosesTakenToday = taken + 1;
    m->lastTaken = today;
    m->quantity--;
    saveStore(s);

    printf("Dose recorded: %d of %d today. %d unit(s) left.\n",
           m->dosesTakenToday, m->dosesPerDay, m->quantity);
    if (m->quantity <= m->minStock)
        printf("Low stock: time to buy more '%s'.\n", m->name);
}

/* Consumer: what needs to be taken today. */
void todaysReminder(const Store *s, Date today) {
    char dateStr[16];
    formatDate(today, dateStr, sizeof dateStr);
    printf("\n--- Reminders for %s ---\n", dateStr);

    if (s->count == 0) {
        printf("No medicines stored yet.\n");
        return;
    }

    int pending = 0;
    for (int i = 0; i < s->count; i++) {
        const Medicine *m = &s->data[i];
        long left = daysLeft(m->expiry, today);

        if (left < 0) {
            printf("  [!] %-20.20s EXPIRED - do not use.\n", m->name);
            continue;
        }

        int remaining = m->dosesPerDay - dosesTakenOn(m, today);
        if (remaining > 0) {
            printf("  [ ] %-20.20s first dose at %s - %d of %d dose(s) still to take\n",
                   m->name, m->doseTime, remaining, m->dosesPerDay);
            pending++;
        } else {
            printf("  [x] %-20.20s all doses done for today\n", m->name);
        }

        if (left <= EXPIRY_WARN_DAYS)
            printf("      Expires in %ld day(s).\n", left);
        if (m->quantity <= m->minStock)
            printf("      Low stock: only %d unit(s) left.\n", m->quantity);
    }

    if (pending == 0) printf("\nNothing left to take today.\n");
    else printf("\n%d medicine(s) still pending today.\n", pending);
}

/* Pharmacist: reduce stock when medicine is sold or dispensed. */
void dispenseStock(Store *s, Date today) {
    if (s->count == 0) {
        printf("No medicines stored yet.\n");
        return;
    }
    displayAll(s, MODE_PHARMACIST, today);

    int id = readInt("\nEnter ID of the medicine to dispense: ", 1, 1000000);
    Medicine *m = findById(s, id);
    if (m == NULL) {
        printf("No medicine with ID %d.\n", id);
        return;
    }
    if (daysLeft(m->expiry, today) < 0) {
        printf("'%s' (batch %s) is expired and cannot be dispensed.\n", m->name, m->batch);
        return;
    }
    if (m->quantity <= 0) {
        printf("'%s' is out of stock.\n", m->name);
        return;
    }

    int qty = readInt("Units to dispense: ", 1, m->quantity);
    m->quantity -= qty;
    saveStore(s);

    printf("Dispensed %d unit(s) of '%s'. %d left.\n", qty, m->name, m->quantity);
    if (m->quantity <= m->minStock)
        printf("Low stock: reorder '%s' from %s.\n", m->name, m->supplier);
}

void sortByExpiry(Store *s) {
    for (int i = 0; i < s->count - 1; i++) {
        for (int j = 0; j < s->count - i - 1; j++) {
            if (compareDates(s->data[j].expiry, s->data[j + 1].expiry) > 0) {
                Medicine temp = s->data[j];
                s->data[j] = s->data[j + 1];
                s->data[j + 1] = temp;
            }
        }
    }
}

/* ---------- Main ---------- */

int main(void) {
    Store store;
    int mode = selectMode();

    initStore(&store, mode == MODE_CONSUMER ? "consumer_data.dat" : "pharmacist_data.dat");
    loadStore(&store);
    printf("\nLoaded %d medicine(s) from '%s'.\n", store.count, store.filename);
    showStartupAlerts(&store, getToday());

    int choice;
    do {
        Date today = getToday();        /* refreshed each time, in case midnight passes */

        showMenu(mode);
        choice = readInt("Enter choice: ", 0, 10);

        switch (choice) {
            case 1:  addMedicine(&store, mode, today);        break;
            case 2:  displayAll(&store, mode, today);         break;
            case 3:  searchByName(&store, mode, today);       break;
            case 4:  updateMedicine(&store, mode);            break;
            case 5:  deleteMedicine(&store);                  break;
            case 6:  expiryReport(&store, mode, today);       break;
            case 7:  lowStockReport(&store, mode, today);     break;
            case 8:
                if (mode == MODE_CONSUMER) takeDose(&store, today);
                else dispenseStock(&store, today);
                break;
            case 9:
                if (mode == MODE_CONSUMER) todaysReminder(&store, today);
                else printf("Invalid choice for pharmacist mode.\n");
                break;
            case 10:
                sortByExpiry(&store);
                saveStore(&store);
                printf("\nSorted by expiry date (earliest first)\n");
                displayAll(&store, mode, today);
                break;
            case 0:
                break;
        }
        if (choice != 0) waitForEnter();
    } while (choice != 0);

    saveStore(&store);
    free(store.data);
    printf("\nData saved to '%s'.\n GET WELL SOON\n goodbye!\n", store.filename);
    return 0;
}