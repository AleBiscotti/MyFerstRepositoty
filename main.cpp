#include <iostream>
#include <vector>
#include <memory>
#include <thread>    // Для потоков
#include <mutex>     // Для мьютексов
#include <algorithm>
#include <stdexcept> // Для исключений

// Искючение для обработки ошибок данных
class DataException : public std::runtime_error {
public:
    // Наследование конструкторов от runtime_error
    using std::runtime_error::runtime_error;
};

// Создание аллокатора
template <typename T>
class LoggingAllocator {
public:
    // Совместимость с контейнерами STL
    using value_type = T;

    // Выделение памяти
    T* allocate(size_t n) {
        std::cout << "Выделяем память для " << n << " элементов\n";
        // Выделение через new
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        std::cout << "Освобождаем память для " << n << " элементов\n";
        // Освобождаем память
        ::operator delete(p);
    }
};

// потокобезопасный вектор
template <typename T, typename Allocator = std::allocator<T>>
class ThreadSafeVector {
private:
    // Внутренний вектор с настраиваемым аллокатором
    std::vector<T, Allocator> data;
    // Мьютекс от гонок данных
    mutable std::mutex mtx;

public:
    // Метод для добавления элементов с поддержкой move-семантики
    void add(T&& value) {
        // Блокируем мьютекс, но он автоматически разблокируется при выходе из области
        std::lock_guard<std::mutex> lock(mtx);
        // Добавление в вектор
        data.push_back(std::forward<T>(value));
    }

    // Элемент по индексу
    T get(size_t index) const {
        // Блокируем мьютекс
        std::lock_guard<std::mutex> lock(mtx);
        // проверка наличия индекса
        if (index >= data.size()) {
            throw DataException("Индекс за пределами вектора");
        }
        return data[index];
    }

    // Размер вектора
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.size();
    }
};

// Главная функция
int main() {
    try {
        // Потокобезопасный вектор с кастомным аллокатором
        ThreadSafeVector<int, LoggingAllocator<int>> safeVec;

        // Будут выплнять потоки
        auto worker = [&safeVec](int id) {
            for (int i = 0; i < 3; ++i) {
                // Поток + 3 значения
                safeVec.add(id * 100 + i);
                // Пауза
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };

        // 5 потоков
        std::vector<std::thread> threads;
        for (int i = 0; i < 5; ++i) {
            // Для каждого потока ID (1-5)
            threads.emplace_back(worker, i + 1);
        }

        // Ждемс завершения всех потоков
        for (auto& t : threads) {
            t.join();
        }

        // Результаты работы
        std::cout << "Всего элементов: " << safeVec.size() << "\n";
        std::cout << "Первый элемент: " << safeVec.get(0) << "\n";

    } catch (const DataException& e) {
        std::cerr << "Ошибка данных: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Общая ошибка: " << e.what() << "\n";
    }

    return 0;
}
