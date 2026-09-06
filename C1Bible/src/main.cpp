#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#ifndef C1BIBLE_FONT_PATH
#define C1BIBLE_FONT_PATH "/storage/c1bible/font12.bin"
#endif
#ifndef C1BIBLE_WIDTH_PATH
#define C1BIBLE_WIDTH_PATH "/storage/c1bible/width12.bin"
#endif
#ifndef C1BIBLE_READER_LINE_HEIGHT
#define C1BIBLE_READER_LINE_HEIGHT 13
#endif
#ifndef C1BIBLE_READER_LINES
#define C1BIBLE_READER_LINES 8
#endif
#ifndef C1BIBLE_HEADER_BOLD
#define C1BIBLE_HEADER_BOLD false
#endif

static constexpr const char *DATA_PATH = "/storage/c1bible/bible.dat";
static constexpr const char *FONT_PATH = C1BIBLE_FONT_PATH;
static constexpr const char *WIDTH_PATH = C1BIBLE_WIDTH_PATH;
static constexpr const char *STATE_PATH = "/storage/c1bible/state.txt";
static constexpr int READER_LINE_HEIGHT = C1BIBLE_READER_LINE_HEIGHT;
static constexpr int READER_LINES = C1BIBLE_READER_LINES;
static constexpr bool HEADER_BOLD = C1BIBLE_HEADER_BOLD;
static volatile sig_atomic_t stopped = 0;
static void stop_handler(int) { stopped = 1; }
static uint64_t milliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

using Frame = std::array<uint8_t, 5624>;
static Frame frame{};
static void pixel(int x, int y, bool black = true) {
    if (x < 0 || x >= 296 || y < 0 || y >= 152) return;
    auto &b = frame[(y / 8) * 296 + x];
    unsigned mask = 0x80 >> (y % 8);
    if (black) b |= mask; else b &= ~mask;
}
static void rect(int x, int y, int w, int h, bool black = true) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) pixel(xx, yy, black);
}
static void outline(int x, int y, int w, int h, bool black = true) {
    rect(x, y, w, 1, black); rect(x, y + h - 1, w, 1, black);
    rect(x, y, 1, h, black); rect(x + w - 1, y, 1, h, black);
}

struct MappedFile {
    int fd = -1;
    size_t size = 0;
    const uint8_t *data = nullptr;
    void open_file(const char *path, size_t expected = 0) {
        fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) throw std::runtime_error(std::string("open failed: ") + path);
        struct stat st{};
        if (fstat(fd, &st) || st.st_size <= 0 || (expected && size_t(st.st_size) != expected))
            throw std::runtime_error(std::string("bad file: ") + path);
        size = size_t(st.st_size);
        void *p = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (p == MAP_FAILED) throw std::runtime_error(std::string("mmap failed: ") + path);
        data = static_cast<const uint8_t *>(p);
    }
    ~MappedFile() {
        if (data) munmap(const_cast<uint8_t *>(data), size);
        if (fd >= 0) close(fd);
    }
    MappedFile() = default;
    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;
};

#pragma pack(push, 1)
struct BibleHeader {
    char magic[8];
    uint32_t version, count, recordsOffset, textOffset, pinyinOffset, syllableOffset, fileSize;
};
struct VerseRecord {
    uint16_t book, chapter, verse, reserved;
    uint32_t textOffset, textLength, pinyinOffset, pinyinLength;
};
#pragma pack(pop)
static_assert(sizeof(BibleHeader) == 36, "header layout");
static_assert(sizeof(VerseRecord) == 24, "record layout");

static const char *BOOKS[66] = {
    "创世记","出埃及记","利未记","民数记","申命记","约书亚记","士师记","路得记",
    "撒母耳记上","撒母耳记下","列王纪上","列王纪下","历代志上","历代志下","以斯拉记",
    "尼希米记","以斯帖记","约伯记","诗篇","箴言","传道书","雅歌","以赛亚书","耶利米书",
    "耶利米哀歌","以西结书","但以理书","何西阿书","约珥书","阿摩司书","俄巴底亚书",
    "约拿书","弥迦书","那鸿书","哈巴谷书","西番雅书","哈该书","撒迦利亚书","玛拉基书",
    "马太福音","马可福音","路加福音","约翰福音","使徒行传","罗马书","哥林多前书",
    "哥林多后书","加拉太书","以弗所书","腓立比书","歌罗西书","帖撒罗尼迦前书",
    "帖撒罗尼迦后书","提摩太前书","提摩太后书","提多书","腓利门书","希伯来书","雅各书",
    "彼得前书","彼得后书","约翰一书","约翰二书","约翰三书","犹大书","启示录"
};
static const char *SHORT_BOOKS[66] = {
    "创","出","利","民","申","书","士","得","撒上","撒下","王上","王下","代上","代下","拉","尼","斯","伯","诗","箴","传","歌",
    "赛","耶","哀","结","但","何","珥","摩","俄","拿","弥","鸿","哈","番","该","亚","玛",
    "太","可","路","约","徒","罗","林前","林后","加","弗","腓","西","帖前","帖后","提前","提后","多","门","来","雅","彼前","彼后","约一","约二","约三","犹","启"
};
static const int CHAPTERS[66] = {
    50,40,27,36,34,24,21,4,31,24,22,25,29,36,10,13,10,42,150,31,12,8,
    66,52,5,48,12,14,3,9,1,4,7,3,3,3,2,14,4,
    28,16,24,21,28,16,16,13,6,6,4,4,5,3,6,4,3,1,13,5,5,3,5,1,1,1,22
};

struct BibleData {
    MappedFile file;
    const BibleHeader *header = nullptr;
    const VerseRecord *records = nullptr;
    const uint8_t *texts = nullptr;
    const uint8_t *pinyins = nullptr;
    std::vector<std::string_view> syllables;
    void load(const char *path = DATA_PATH) {
        file.open_file(path);
        if (file.size < sizeof(BibleHeader)) throw std::runtime_error("Bible database too small");
        header = reinterpret_cast<const BibleHeader *>(file.data);
        if (memcmp(header->magic, "C1BIBL2\0", 8) || header->version != 2 ||
            header->count != 31021 || header->fileSize != file.size ||
            header->recordsOffset != sizeof(BibleHeader) ||
            header->textOffset != header->recordsOffset + header->count * sizeof(VerseRecord) ||
            header->textOffset > header->pinyinOffset || header->pinyinOffset > header->syllableOffset ||
            header->syllableOffset > file.size)
            throw std::runtime_error("Invalid Bible database");
        records = reinterpret_cast<const VerseRecord *>(file.data + header->recordsOffset);
        texts = file.data + header->textOffset;
        pinyins = file.data + header->pinyinOffset;
        for (uint32_t i = 0; i < header->count; ++i) {
            const auto &r = records[i];
            if (r.book < 1 || r.book > 66 || r.chapter < 1 || r.chapter > CHAPTERS[r.book - 1] ||
                header->textOffset + r.textOffset + r.textLength > header->pinyinOffset ||
                header->pinyinOffset + r.pinyinOffset + r.pinyinLength > header->syllableOffset)
                throw std::runtime_error("Invalid verse record");
        }
        const char *p = reinterpret_cast<const char *>(file.data + header->syllableOffset);
        const char *end = reinterpret_cast<const char *>(file.data + file.size);
        while (p < end) {
            const char *line = p; while (p < end && *p != '\n') ++p;
            if (p > line) syllables.emplace_back(line, p - line);
            if (p < end) ++p;
        }
    }
    std::string_view text(const VerseRecord &r) const {
        return {reinterpret_cast<const char *>(texts + r.textOffset), r.textLength};
    }
    std::string_view pinyin(const VerseRecord &r) const {
        return {reinterpret_cast<const char *>(pinyins + r.pinyinOffset), r.pinyinLength};
    }
    uint32_t lower_bound(int book, int chapter, int verse = 0) const {
        uint32_t lo = 0, hi = header->count;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo) / 2;
            const auto &r = records[mid];
            bool less = r.book < book || (r.book == book && (r.chapter < chapter ||
                (r.chapter == chapter && r.verse < verse)));
            if (less) lo = mid + 1; else hi = mid;
        }
        return lo;
    }
    std::pair<uint32_t, uint32_t> chapter_range(int book, int chapter) const {
        uint32_t first = lower_bound(book, chapter, 0);
        uint32_t last = (chapter < CHAPTERS[book - 1]) ? lower_bound(book, chapter + 1, 0)
            : (book < 66 ? lower_bound(book + 1, 1, 0) : header->count);
        return {first, last};
    }
    std::string search_pattern(std::string_view query) const {
        std::string pattern = "'"; size_t pos = 0;
        while (pos < query.size()) {
            std::string_view best;
            for (auto syllable : syllables)
                if (syllable.size() > best.size() && pos + syllable.size() <= query.size() &&
                    query.substr(pos, syllable.size()) == syllable) best = syllable;
            if (best.empty()) return {};
            pattern.append(best.data(), best.size()); pattern.push_back('\''); pos += best.size();
        }
        return pattern;
    }
    std::vector<uint32_t> search(std::string_view query, size_t limit = 200) const {
        std::vector<uint32_t> result;
        std::string pattern = search_pattern(query); if (pattern.empty()) return result;
        for (uint32_t i = 0; i < header->count && result.size() < limit; ++i)
            if (pinyin(records[i]).find(pattern) != std::string_view::npos) result.push_back(i);
        return result;
    }
};

static uint32_t next_rune(const char *&p, const char *end) {
    if (p >= end) return 0;
    uint32_t c = static_cast<unsigned char>(*p++);
    if (c < 0x80) return c;
    int n = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
    c &= (1u << (6 - n)) - 1;
    while (n-- && p < end) c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
    return c;
}

struct Font {
    MappedFile glyphs, widths;
    void load(const char *fontPath = FONT_PATH, const char *widthPath = WIDTH_PATH) {
        glyphs.open_file(fontPath, 65536u * 32u);
        widths.open_file(widthPath, 65536u);
    }
    int width(uint32_t cp) const {
        if (cp > 65535 || cp < 32) return 8;
        int w = widths.data[cp];
        return w ? w : 8;
    }
    int measure(std::string_view s) const {
        int result = 0; const char *p = s.data(), *end = p + s.size();
        while (p < end) result += width(next_rune(p, end));
        return result;
    }
    std::string fit(std::string_view s, int maxWidth) const {
        std::string result; int used = 0; const char *p = s.data(), *end = p + s.size();
        while (p < end) {
            const char *start = p; uint32_t cp = next_rune(p, end); int w = width(cp);
            if (used + w > maxWidth) break;
            result.append(start, p - start); used += w;
        }
        return result;
    }
    void draw(int x, int y, std::string_view s, bool black = true, bool bold = false) const {
        const char *p = s.data(), *end = p + s.size();
        while (p < end) {
            uint32_t cp = next_rune(p, end); if (cp > 65535) cp = 0xFFFD;
            int w = width(cp); if (x + w > 296) return;
            const uint8_t *g = glyphs.data + cp * 32;
            for (int row = 0; row < 16; ++row) {
                uint16_t bits = uint16_t(g[row * 2] << 8) | g[row * 2 + 1];
                for (int col = 0; col < w; ++col) if (bits & (0x8000u >> col)) {
                    pixel(x + col, y + row, black);
                    if (bold && x + col + 1 < 296) pixel(x + col + 1, y + row, black);
                }
            }
            x += w;
        }
    }
};

static std::string format(const char *pattern, int a, int b = 0, int c = 0) {
    char out[96]; snprintf(out, sizeof(out), pattern, a, b, c); return out;
}
static void button(const Font &font, int x, int y, int w, std::string_view label, bool selected = false) {
    if (selected) rect(x, y, w, 19); else outline(x, y, w, 19);
    std::string fitted = font.fit(label, w - 6);
    int tx = x + std::max(3, (w - font.measure(fitted)) / 2);
    font.draw(tx, y + 2, fitted, !selected);
}
static void book_mark(int x, int y) {
    outline(x, y + 2, 19, 13); rect(x + 9, y + 2, 1, 13);
    rect(x + 3, y + 5, 5, 1); rect(x + 5, y + 3, 1, 7);
}

struct ReadLine { std::string text; int verse = 1; };
enum class View { Books, Chapters, Reader, Search, Results };

struct UI {
    BibleData &db;
    Font &font;
    View view = View::Books, searchReturn = View::Books;
    int book = 0, chapter = 1, page = 0;
    int lastBook = 0, lastChapter = 1, lastVerse = 1;
    int chapterPage = 0;
    std::vector<ReadLine> lines;
    std::string query;
    std::vector<uint32_t> results;
    int resultSelected = 0;
    std::string notice;
    std::string statePath;
    bool dirty = true;

    UI(BibleData &d, Font &f, std::string path = STATE_PATH) : db(d), font(f), statePath(std::move(path)) { load_state(); }
    void load_state() {
        FILE *fp = fopen(statePath.c_str(), "rb"); if (!fp) return;
        int b, c, v;
        if (fscanf(fp, "C1BIBLE1 %d %d %d", &b, &c, &v) == 3 &&
            b >= 1 && b <= 66 && c >= 1 && c <= CHAPTERS[b - 1] && v >= 1) {
            lastBook = b - 1; lastChapter = c; lastVerse = v;
        }
        fclose(fp);
    }
    void save_state() {
        std::string tmp = statePath + ".tmp";
        FILE *fp = fopen(tmp.c_str(), "wb"); if (!fp) { notice = "阅读进度保存失败"; return; }
        fprintf(fp, "C1BIBLE1 %d %d %d\n", lastBook + 1, lastChapter, lastVerse);
        if (fclose(fp) || rename(tmp.c_str(), statePath.c_str())) notice = "阅读进度保存失败";
    }
    void add_wrapped(int verse, std::string_view body) {
        std::string current = std::to_string(verse) + " ";
        std::string continuation = "  ";
        int used = font.measure(current);
        const char *p = body.data(), *end = p + body.size();
        while (p < end) {
            const char *start = p; uint32_t cp = next_rune(p, end); int w = font.width(cp);
            if (used + w > 280 && current.size() > continuation.size()) {
                lines.push_back({current, verse}); current = continuation; used = font.measure(current);
            }
            current.append(start, p - start); used += w;
        }
        lines.push_back({current, verse});
    }
    void open_reader(int targetVerse = 1) {
        lines.clear(); int targetLine = 0;
        auto range = db.chapter_range(book + 1, chapter);
        for (uint32_t i = range.first; i < range.second; ++i) {
            if (db.records[i].verse == targetVerse) targetLine = int(lines.size());
            add_wrapped(db.records[i].verse, db.text(db.records[i]));
        }
        page = std::max(0, targetLine / READER_LINES);
        view = View::Reader; remember_page();
    }
    int pages() const { return std::max(1, int((lines.size() + READER_LINES - 1) / READER_LINES)); }
    void remember_page() {
        if (view != View::Reader || lines.empty()) return;
        int line = std::min(int(lines.size()) - 1, page * READER_LINES);
        lastBook = book; lastChapter = chapter; lastVerse = lines[line].verse; save_state();
    }
    void change_chapter(int delta) {
        if (delta < 0) {
            if (chapter > 1) --chapter;
            else if (book > 0) { --book; chapter = CHAPTERS[book]; }
            else return;
        } else {
            if (chapter < CHAPTERS[book]) ++chapter;
            else if (book < 65) { ++book; chapter = 1; }
            else return;
        }
        open_reader(1);
    }
    void enter_search() { searchReturn = view; view = View::Search; notice.clear(); }
    void perform_search() {
        if (query.empty()) { notice = "请先输入拼音关键词"; return; }
        results = db.search(query); resultSelected = 0;
        notice = results.empty() ? "没有找到相关经文" : (results.size() == 200 ? "结果较多，仅显示前200条" : "");
        view = View::Results;
    }
    static char letter_key(int code) {
        switch (code) {
            case KEY_A:return 'a';case KEY_B:return 'b';case KEY_C:return 'c';case KEY_D:return 'd';
            case KEY_E:return 'e';case KEY_F:return 'f';case KEY_G:return 'g';case KEY_H:return 'h';
            case KEY_I:return 'i';case KEY_J:return 'j';case KEY_K:return 'k';case KEY_L:return 'l';
            case KEY_M:return 'm';case KEY_N:return 'n';case KEY_O:return 'o';case KEY_P:return 'p';
            case KEY_Q:return 'q';case KEY_R:return 'r';case KEY_S:return 's';case KEY_T:return 't';
            case KEY_U:return 'u';case KEY_V:return 'v';case KEY_W:return 'w';case KEY_X:return 'x';
            case KEY_Y:return 'y';case KEY_Z:return 'z'; default:return 0;
        }
    }
    bool key(int code) {
        dirty = true;
        if (code == KEY_HOME) return false;
        if (view == View::Search) {
            if (code == KEY_BACK || code == KEY_ESC) { view = searchReturn; notice.clear(); return false; }
            if (code == KEY_BACKSPACE || code == KEY_DELETE) { if (!query.empty()) query.pop_back(); notice.clear(); return false; }
            if (code == KEY_ENTER || code == KEY_OK) { perform_search(); return false; }
            if (code == KEY_C && query.empty()) return false;
            char c = letter_key(code); if (c && query.size() < 30) { query.push_back(c); notice.clear(); }
            return false;
        }
        if (view == View::Results) {
            if (code == KEY_BACK || code == KEY_ESC || code == KEY_F) { view = View::Search; return false; }
            if (code == KEY_D) { view = View::Books; return false; }
            if (results.empty()) return false;
            int n = int(results.size());
            if (code == KEY_UP) resultSelected = std::max(0, resultSelected - 1);
            else if (code == KEY_DOWN) resultSelected = std::min(n - 1, resultSelected + 1);
            else if (code == KEY_LEFT) resultSelected = std::max(0, resultSelected - 4);
            else if (code == KEY_RIGHT) resultSelected = std::min(n - 1, resultSelected + 4);
            else if (code == KEY_ENTER || code == KEY_OK) {
                const auto &r = db.records[results[resultSelected]];
                book = r.book - 1; chapter = r.chapter; open_reader(r.verse);
            }
            return false;
        }
        if (code == KEY_F) { enter_search(); return false; }
        if (view == View::Books) {
            if (code == KEY_BACK || code == KEY_ESC) return true;
            if (code == KEY_Q) book = 0;
            else if (code == KEY_W) book = 39;
            else if (code == KEY_UP) book = std::max(0, book - 2);
            else if (code == KEY_DOWN) book = std::min(65, book + 2);
            else if (code == KEY_LEFT) book = std::max(0, book - 1);
            else if (code == KEY_RIGHT) book = std::min(65, book + 1);
            else if (code == KEY_C) { book = lastBook; chapter = lastChapter; open_reader(lastVerse); }
            else if (code == KEY_ENTER || code == KEY_OK) { chapter = 1; chapterPage = 0; view = View::Chapters; }
            return false;
        }
        if (view == View::Chapters) {
            if (code == KEY_BACK || code == KEY_ESC || code == KEY_D) { view = View::Books; return false; }
            int maxChapter = CHAPTERS[book];
            if (code == KEY_UP) chapter = std::max(1, chapter - 10);
            else if (code == KEY_DOWN) chapter = std::min(maxChapter, chapter + 10);
            else if (code == KEY_LEFT) chapter = std::max(1, chapter - 1);
            else if (code == KEY_RIGHT) chapter = std::min(maxChapter, chapter + 1);
            else if (code == KEY_PAGEUP) chapter = std::max(1, chapter - 50);
            else if (code == KEY_PAGEDOWN) chapter = std::min(maxChapter, chapter + 50);
            else if (code == KEY_ENTER || code == KEY_OK) open_reader(1);
            chapterPage = (chapter - 1) / 50;
            return false;
        }
        if (view == View::Reader) {
            if (code == KEY_BACK || code == KEY_ESC) { view = View::Chapters; chapterPage = (chapter - 1) / 50; return false; }
            if (code == KEY_D) { view = View::Books; return false; }
            if (code == KEY_PAGEUP) { change_chapter(-1); return false; }
            if (code == KEY_PAGEDOWN) { change_chapter(1); return false; }
            if (code == KEY_LEFT || code == KEY_UP) page = std::max(0, page - 1);
            else if (code == KEY_RIGHT || code == KEY_DOWN || code == KEY_ENTER || code == KEY_OK) {
                if (page + 1 < pages()) ++page;
                else { change_chapter(1); return false; }
            }
            remember_page(); return false;
        }
        return false;
    }
    void header(std::string_view title, std::string_view right = {}) {
        book_mark(8, 1); font.draw(34, 1, font.fit(title, right.empty() ? 252 : 190), true, HEADER_BOLD);
        if (!right.empty()) font.draw(288 - font.measure(right), 1, right);
        rect(8, 20, 280, 1);
    }
    void render_books() {
        int start = (book / 12) * 12;
        std::string side = book < 39 ? "旧约" : "新约";
        header("圣经 · 和合本", side + format(" %d/6", book < 39 ? book / 12 + 1 : (book - 39) / 12 + 1));
        for (int i = 0; i < 12 && start + i < 66; ++i) {
            int index = start + i, col = i % 2, row = i / 2;
            int x = 8 + col * 142, y = 24 + row * 17;
            bool selected = index == book;
            if (selected) rect(x, y, 138, 16);
            font.draw(x + 4, y, font.fit(BOOKS[index], 130), !selected);
        }
        button(font, 8, 131, 91, "Enter 章节");
        button(font, 103, 131, 88, "C 续读");
        button(font, 195, 131, 93, "F 搜索");
    }
    void render_chapters() {
        int totalPages = (CHAPTERS[book] + 49) / 50;
        header(BOOKS[book], format("%d章  %d/%d", CHAPTERS[book], chapterPage + 1, totalPages));
        int start = chapterPage * 50 + 1;
        for (int i = 0; i < 50 && start + i <= CHAPTERS[book]; ++i) {
            int number = start + i, col = i % 10, row = i / 10;
            int x = 8 + col * 28, y = 25 + row * 20;
            bool selected = number == chapter;
            if (selected) rect(x, y, 26, 18);
            std::string label = std::to_string(number);
            font.draw(x + (26 - font.measure(label)) / 2, y + 1, label, !selected);
        }
        button(font, 8, 131, 92, "Back 目录");
        button(font, 104, 131, 91, "Enter 阅读");
        button(font, 199, 131, 89, "F 搜索");
    }
    void render_reader() {
        std::string title = std::string(BOOKS[book]) + " " + std::to_string(chapter);
        header(title, format("%d / %d", page + 1, pages()));
        for (int i = 0; i < READER_LINES; ++i) {
            int index = page * READER_LINES + i;
            if (index < int(lines.size())) font.draw(8, 23 + i * READER_LINE_HEIGHT, lines[index].text);
        }
        rect(8, 130, 280, 1);
        font.draw(8, 134, "← →翻页");
        std::string hint = "Pg换章  F搜索";
        font.draw(288 - font.measure(hint), 134, hint);
    }
    void render_search() {
        header("经文搜索", "拼音输入");
        outline(8, 30, 280, 25);
        std::string shown = query.empty() ? "例如 yesu / endian" : query + "_";
        font.draw(14, 35, font.fit(shown, 268));
        font.draw(8, 67, "输入中文词语的完整拼音");
        font.draw(8, 87, "Backspace 删除  Enter 搜索");
        if (!notice.empty()) font.draw(8, 108, font.fit(notice, 280), true, true);
        button(font, 8, 131, 93, "Back 返回");
        button(font, 105, 131, 183, "Enter 开始搜索", true);
    }
    void render_results() {
        std::string right = results.empty() ? "0 条" : format("%d / %d", resultSelected + 1, int(results.size()));
        header(std::string("搜索 · ") + query, right);
        if (results.empty()) {
            font.draw(58, 52, "没有找到相关经文", true, true);
            font.draw(58, 77, "Back 修改关键词");
        } else {
            int start = (resultSelected / 4) * 4;
            for (int i = 0; i < 4 && start + i < int(results.size()); ++i) {
                int pos = start + i, y = 24 + i * 25;
                const auto &r = db.records[results[pos]]; bool selected = pos == resultSelected;
                if (selected) outline(8, y, 280, 23);
                std::string ref = std::string(SHORT_BOOKS[r.book - 1]) + " " + std::to_string(r.chapter) + ":" + std::to_string(r.verse);
                font.draw(13, y + 3, ref, true, selected);
                int x = 88; font.draw(x, y + 3, font.fit(db.text(r), 195));
            }
        }
        button(font, 8, 131, 93, "Back 改词");
        button(font, 105, 131, 88, "Enter 阅读");
        button(font, 197, 131, 91, "D 目录");
    }
    void render() {
        frame.fill(0);
        switch (view) {
            case View::Books: render_books(); break;
            case View::Chapters: render_chapters(); break;
            case View::Reader: render_reader(); break;
            case View::Search: render_search(); break;
            case View::Results: render_results(); break;
        }
    }
};

struct Inputs {
    std::vector<pollfd> fds;
    std::array<bool, KEY_MAX + 1> down{};
    Inputs() { try {
        for (auto path : {"/dev/input/event0", "/dev/input/event1"}) {
            int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0) throw std::runtime_error("open input failed");
            if (ioctl(fd, EVIOCGRAB, 1) < 0) { close(fd); throw std::runtime_error("exclusive input grab failed"); }
            fds.push_back({fd, POLLIN, 0});
        }
    } catch (...) { for (auto p : fds) close(p.fd); throw; } }
    ~Inputs() { for (auto p : fds) close(p.fd); }
    template<class F> void poll_events(F callback, int timeout) {
        if (poll(fds.data(), fds.size(), timeout) <= 0) return;
        for (auto p : fds) if (p.revents & POLLIN) {
            input_event ev;
            while (read(p.fd, &ev, sizeof(ev)) == sizeof(ev)) if (ev.type == EV_KEY && ev.code <= KEY_MAX) {
                down[ev.code] = ev.value != 0; callback(ev.code, ev.value);
            }
        }
    }
    void drain() {
        uint64_t quiet = 0;
        while (!stopped) {
            poll_events([](int, int){}, 20);
            if (std::any_of(down.begin(), down.end(), [](bool b){ return b; })) quiet = 0;
            else if (!quiet) quiet = milliseconds();
            else if (milliseconds() - quiet >= 150) return;
        }
    }
};

static void write_frame() {
    int fd = open("/dev/epaper_lcd", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) throw std::runtime_error("display open failed");
    auto n = write(fd, frame.data(), frame.size()); close(fd);
    if (n != ssize_t(frame.size())) throw std::runtime_error("display write failed");
}
static bool dump(const char *path) {
    FILE *f = fopen(path, "wb"); if (!f) return false;
    bool ok = fwrite(frame.data(), 1, frame.size(), f) == frame.size();
    if (fclose(f)) ok = false; return ok;
}
static int selftest(BibleData &db, Font &font) {
    if (db.header->count != 31021) return 1;
    auto first = db.chapter_range(1, 1);
    if (first.second - first.first != 31 || db.text(db.records[first.first]).find("起初") == std::string_view::npos) return 2;
    auto last = db.chapter_range(66, 22);
    if (last.second <= last.first || db.records[last.second - 1].verse != 21) return 3;
    auto hits = db.search("yesu", 300);
    if (hits.size() < 100) return 4;
    UI ui(db, font, "/tmp/c1bible-selftest-state.txt");
    if (ui.key(KEY_HOME)) return 5;
    ui.book = 42; ui.chapter = 3; ui.open_reader(16);
    if (ui.view != View::Reader || ui.lines.empty()) return 6;
    ui.query = "endian"; ui.perform_search();
    if (ui.results.empty()) return 7;
    ui.book = 42; ui.chapter = 3; ui.open_reader(1); ui.page = ui.pages() - 1;
    ui.key(KEY_RIGHT);
    if (ui.book != 42 || ui.chapter != 4 || ui.page != 0 || ui.view != View::Reader) return 8;
    ui.book = 39; ui.chapter = 28; ui.open_reader(1); ui.page = ui.pages() - 1;
    ui.key(KEY_ENTER);
    if (ui.book != 40 || ui.chapter != 1 || ui.page != 0) return 9;
    ui.book = 65; ui.chapter = 22; ui.open_reader(1); ui.page = ui.pages() - 1;
    int finalPage = ui.page; ui.key(KEY_RIGHT);
    if (ui.book != 65 || ui.chapter != 22 || ui.page != finalPage) return 10;
    remove("/tmp/c1bible-selftest-state.txt");
    puts("PASS: 66 books, 1189 chapters, 31021 verses, pinyin search, continuous chapter paging, HOME isolation");
    return 0;
}

int main(int argc, char **argv) { try {
    BibleData db; db.load(); Font font; font.load();
    if (argc == 2 && std::string(argv[1]) == "--version") {
        puts("C1Bible 1.1.1 / Fusion Pixel 12px / CUVS / 31021 verses / 1-bit"); return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--selftest") return selftest(db, font);
    UI ui(db, font, argc == 3 ? "/tmp/c1bible-preview-state.txt" : STATE_PATH);
    if (argc == 3) {
        std::string mode = argv[1];
        if (mode == "--preview-dir") {}
        else if (mode == "--preview-reader") { ui.book = 42; ui.chapter = 3; ui.open_reader(16); }
        else if (mode == "--preview-reader-start") { ui.book = 42; ui.chapter = 3; ui.open_reader(1); }
        else if (mode == "--preview-search") { ui.query = "yesu"; ui.perform_search(); }
        else return 2;
        ui.render(); return dump(argv[2]) ? 0 : 1;
    }
    signal(SIGINT, stop_handler); signal(SIGTERM, stop_handler); signal(SIGHUP, stop_handler);
    Inputs inputs;
    uint64_t start = milliseconds(); while (milliseconds() - start < 180) inputs.poll_events([](int, int){}, 20);
    Frame previous{}; bool valid = false, exit = false; uint64_t lastWrite = 0, lastRepeat = 0;
    while (!stopped && !exit) {
        inputs.poll_events([&](int code, int value) {
            if (exit) return;
            bool repeatable = code == KEY_UP || code == KEY_DOWN || code == KEY_LEFT || code == KEY_RIGHT || code == KEY_BACKSPACE;
            if (value == 1 || (value == 2 && repeatable && milliseconds() - lastRepeat >= 150)) {
                lastRepeat = milliseconds(); exit = ui.key(code);
            }
        }, 30);
        if (!exit && ui.dirty && milliseconds() - lastWrite >= 90) {
            ui.render();
            if (!valid || frame != previous) { write_frame(); previous = frame; valid = true; lastWrite = milliseconds(); }
            ui.dirty = false;
        }
    }
    inputs.drain(); return 0;
} catch (const std::exception &e) {
    fprintf(stderr, "c1bible: %s\n", e.what()); return 1;
} }
