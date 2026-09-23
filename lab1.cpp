#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Point2D
{
    double x = 0.0;
    double y = 0.0;

    auto operator<=>(const Point2D&) const = default;
};

class AffineTransform2D
{
public:
    AffineTransform2D(double a, double b, double c, double d, double e, double f)
        : row1_{a, b}, row2_{c, d}, shift_{e, f}
    {
    }

    Point2D operator()(const Point2D &p) const
    {
        const array<double, 2> v{p.x, p.y};
        const double newX = inner_product(row1_.begin(), row1_.end(), v.begin(),
                                           0.0, plus<>(), multiplies<>()) + shift_.x;
        const double newY = inner_product(row2_.begin(), row2_.end(), v.begin(),
                                           0.0, plus<>(), multiplies<>()) + shift_.y;
        return Point2D{newX, newY};
    }

private:
    array<double, 2> row1_;  
    array<double, 2> row2_;  
    Point2D shift_;          
};

class RandomAffineTransformer
{
public:
    explicit RandomAffineTransformer(vector<AffineTransform2D> transforms)
        : transforms_(move(transforms)),
          engine_(random_device{}()),
          indexDistribution_(0, transforms_.size() - 1)
    {
        if (transforms_.empty())
        {
            throw invalid_argument("Transformer: список перетворень не може бути порожнім");
        }
    }

    Point2D operator()(const Point2D &p)
    {
        const size_t chosen = indexDistribution_(engine_);
        return transforms_[chosen](p);
    }

private:
    vector<AffineTransform2D> transforms_;
    mt19937 engine_;
    uniform_int_distribution<size_t> indexDistribution_;
};

struct InputData
{
    size_t pointCount = 0;
    Point2D start;
    vector<AffineTransform2D> transforms;
};

InputData readInputData(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        throw runtime_error("Не вдалося відкрити файл: " + filename);
    }

    InputData data;
    string line;
    stringstream ss;

    if (!getline(file, line))
    {
        throw runtime_error("Вхідний файл порожній");
    }
    
    ss.str(line);
    long long n = 0;
    if (!(ss >> n) || n <= 0)
    {
        throw runtime_error("Помилка: N має бути додатним цілим числом");
    }
    data.pointCount = static_cast<size_t>(n);

    if (!getline(file, line))
    {
        throw runtime_error("Помилка: відсутній рядок з початковою точкою");
    }
    
    ss.clear();
    ss.str(line);
    if (!(ss >> data.start.x >> data.start.y))
    {
        throw runtime_error("Помилка: некоректний формат початкової точки (очікується 'x y')");
    }

    while (getline(file, line))
    {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos)
        {
            continue;
        }

        ss.clear();
        ss.str(line);

        double a, b, c, d, e, f;
        if (!(ss >> a >> b >> c >> d >> e >> f))
        {
            throw runtime_error("Помилка: афінне перетворення має містити 6 дійсних чисел");
        }

        data.transforms.emplace_back(a, b, c, d, e, f);
    }

    if (data.transforms.empty())
    {
        throw runtime_error("Помилка: у файлі не знайдено жодного афінного перетворення");
    }

    return data;
}

vector<Point2D> generateSequence(const Point2D &start, RandomAffineTransformer &transformer,
                                  size_t count)
{
    vector<Point2D> points;
    points.reserve(count);
    points.push_back(start);

    if (count > 1)
    {
        generate_n(back_inserter(points), count - 1,
                   [&transformer, current = start]() mutable
                   {
                       current = transformer(current);
                       return current;
                   });
    }

    return points;
}

void writeOutputData(const string &filename, const vector<Point2D> &points)
{
    ofstream out(filename);
    if (!out)
    {
        throw runtime_error("Не вдалося створити вихідний файл: " + filename);
    }

    out << fixed << setprecision(6);
    for (const Point2D &p : points)
    {
        out << p.x << ' ' << p.y << '\n';
    }
}

int main(int argc, char *argv[])
{
    const string inputFile = (argc > 1) ? argv[1] : "input.txt";
    const string outputFile = (argc > 2) ? argv[2] : "output.txt";

    try
    {
        InputData data = readInputData(inputFile);

        cout << "Зчитано перетворень: " << data.transforms.size()
             << "\nПочаткова точка: (" << data.start.x << ", " << data.start.y << ")"
             << "\nГенерація " << data.pointCount << " точок...\n";

        RandomAffineTransformer transformer(data.transforms);
        vector<Point2D> points = generateSequence(data.start, transformer, data.pointCount);

        writeOutputData(outputFile, points);

        cout << "Успішно записано у файл: " << outputFile << '\n';
    }
    catch (const exception &ex)
    {
        cerr << "Помилка виконання: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}