#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// cтруктура  номинал + количество
struct Bill
{
    int denomination; // номинал
    int count;        // количество
};

// вспомогательные функции для ручного парсинга JSON

// убрать пробелы, табы, переносы строк
string trim(const string& s)
{
    int start = 0;
    int end = (int)s.size() - 1;
    while (start <= end &&
           (s[start] == ' ' || s[start] == '\t' ||
            s[start] == '\n' || s[start] == '\r'))
        start++;
    while (end >= start &&
           (s[end] == ' ' || s[end] == '\t' ||
            s[end] == '\n' || s[end] == '\r'))
        end--;
    if (start > end) return "";
    return s.substr(start, end - start + 1);
}

// поиск значения по ключу в JSON строке
string findValue(const string& json, const string& key)
{
    // Ищем ключ
    string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return "";

    // после ключа поиск :
    pos = json.find(':', pos + searchKey.size());
    if (pos == string::npos) return "";
    pos++; 

    // пропуск пробелов
    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\t' ||
            json[pos] == '\n' || json[pos] == '\r'))
        pos++;

    if (pos >= json.size()) return "";

    // если начинается с [ - поиск парную ]
    if (json[pos] == '[')
    {
        int depth = 0;
        size_t start = pos;
        for (size_t i = pos; i < json.size(); i++)
        {
            if (json[i] == '[') depth++;
            if (json[i] == ']') depth--;
            if (depth == 0)
                return json.substr(start, i - start + 1);
        }
        return "";
    }

    // если начинается с " - строка
    if (json[pos] == '"')
    {
        size_t start = pos + 1;
        size_t end = json.find('"', start);
        if (end == string::npos) return "";
        return json.substr(start, end - start);
    }

    // иначе - число
    size_t start = pos;
    while (pos < json.size() &&
           json[pos] != ',' && json[pos] != '}' &&
           json[pos] != '\n' && json[pos] != '\r')
        pos++;
    return trim(json.substr(start, pos - start));
}

// парсить массив пар [[a,b],[c,d],...]
// возврат вектор Bill
vector<Bill> parsePairs(const string& arrStr)
{
    vector<Bill> result;
    string s = trim(arrStr);

    // убрать внешние скобки
    if (s.empty() || s[0] != '[') return result;
    s = s.substr(1, s.size() - 2);

    // ищем каждую пару номинал, количество
    size_t i = 0;
    while (i < s.size())
    {
        // поиск [
        size_t start = s.find('[', i);
        if (start == string::npos) break;

        // поиск парной ]
        size_t end = s.find(']', start);
        if (end == string::npos) break;

        // получение содержимого пары: 100,3
        string pair = s.substr(start + 1, end - start - 1);

        // разбивка по ,
        size_t comma = pair.find(',');
        if (comma != string::npos)
        {
            Bill b;
            b.denomination = stoi(trim(pair.substr(0, comma)));
            b.count        = stoi(trim(pair.substr(comma + 1)));
            result.push_back(b);
        }

        i = end + 1;
    }
    return result;
}
// чтение всго файла в строку
string readFile(const string& filename)
{
    ifstream f(filename);
    if (!f.is_open())
    {
        cerr << "Ошибка: не удалось открыть файл " << filename << endl;
        return "";
    }
    stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

// запись результата 
void writeOutput(const string& filename, const vector<Bill>& dispense)
{
    ofstream f(filename);
    if (!f.is_open())
    {
        cerr << "Ошибка: не удалось открыть файл " << filename << endl;
        return;
    }

    f << "{\n";
    f << "  \"dispense\": [";

    // фильтр купюры
    vector<Bill> nonZero;
    for (const Bill& b : dispense)
        if (b.count > 0)
            nonZero.push_back(b);

    for (size_t i = 0; i < nonZero.size(); i++)
    {
        f << "[" << nonZero[i].denomination
          << "," << nonZero[i].count << "]";
        if (i + 1 < nonZero.size()) f << ",";
    }

    f << "]\n}\n";
}

// bills - отсортированный массив купюр
// idx - индекс в bills
// remain - оставшаяся сумма
// chosen - сколько берём купюр каждого номинала
bool solve(const vector<Bill>& bills,
           int idx,
           int remain,
           vector<int>& chosen)
{
    // если сумма набрана
    if (remain == 0) return true;

    // если вышли за пределы массива или сумма отрицательная
    if (idx >= (int)bills.size() || remain < 0) return false;

    const Bill& b = bills[idx];

    // берем от максимального количества до 0
    int maxTake = min(b.count, remain / b.denomination);

    for (int take = maxTake; take >= 0; take--)
    {
        chosen[idx] = take;
        if (solve(bills, idx + 1,
                  remain - take * b.denomination,
                  chosen))
            return true;
    }

    chosen[idx] = 0;
    return false;
}
int main()
{
    // чтение input.json
    string jsonIn = readFile("input.json");
    if (jsonIn.empty()) return 1;

    // парсинг поля
    string walletStr   = findValue(jsonIn, "wallet");
    string amountStr   = findValue(jsonIn, "amount");
    string strategyStr = findValue(jsonIn, "strategy");

    if (walletStr.empty() || amountStr.empty() || strategyStr.empty())
    {
        cerr << "Ошибка: неверный формат input.json" << endl;
        return 1;
    }

    vector<Bill> wallet = parsePairs(walletStr);
    int amount          = stoi(trim(amountStr));
    string strategy     = trim(strategyStr);

    // отладочный вывод входных данных
    cout << "=== Входные данные ===" << endl;
    cout << "Сумма: " << amount << endl;
    cout << "Стратегия: " << strategy << endl;
    cout << "Кошелёк:" << endl;
    for (const Bill& b : wallet)
        cout << "  номинал=" << b.denomination
             << " количество=" << b.count << endl;

    // сортировка купюры по стратегии min или max
    vector<Bill> bills = wallet;

    if (strategy == "MAX")
    {
        sort(bills.begin(), bills.end(),
             [](const Bill& a, const Bill& b)
             { return a.denomination > b.denomination; });
    }
    else
    {
        sort(bills.begin(), bills.end(),
             [](const Bill& a, const Bill& b)
             { return a.denomination < b.denomination; });
    }

    cout << "\nКупюры после сортировки (" << strategy << "):" << endl;
    for (const Bill& b : bills)
        cout << "  номинал=" << b.denomination
             << " количество=" << b.count << endl;

    // рекурсивный поиск решения
    vector<int> chosen(bills.size(), 0);
    bool found = solve(bills, 0, amount, chosen);

    vector<Bill> dispense;

    if (found)
    {
        cout << "\n=== Решение найдено ===" << endl;
        for (int i = 0; i < (int)bills.size(); i++)
        {
            if (chosen[i] > 0)
            {
                Bill b;
                b.denomination = bills[i].denomination;
                b.count        = chosen[i];
                dispense.push_back(b);
                cout << "  номинал=" << b.denomination
                     << " количество=" << b.count << endl;
            }
        }

        // проверка суммы
        int total = 0;
        for (const Bill& b : dispense)
            total += b.denomination * b.count;
        cout << "Итоговая сумма: " << total << endl;
    }
    else
    {
        cout << "\nРешение не найдено → dispense = []" << endl;
    }

    // запись output.json
    writeOutput("output.json", dispense);
    cout << "\nРезультат записан в output.json" << endl;

    return 0;
}