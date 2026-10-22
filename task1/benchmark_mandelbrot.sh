#!/bin/bash

# Настройки
EXEC="./mandelbrot"
TEND="1000000"
DT="86400"
THREADS=(1 2 4 6 8)
RUNS=5

# Проверка, существует ли скомпилированная программа и файл с данными
if [ ! -f "$EXEC" ]; then
    echo "Ошибка: Файл $EXEC не найден! Скомпилируй программу."
    exit 1
fi


echo "Начинаем тестирование ($RUNS запусков для каждого числа потоков)..."
echo "--------------------------------------------------------------------------------"
printf "%-10s | %-15s | %-15s | %-15s\n" "Потоки" "Среднее время (с)" "Ускорение (S)" "Эффективность (E)"
echo "--------------------------------------------------------------------------------"

time_1=0

for t in "${THREADS[@]}"; do
    total_time=0
    
    for ((i=1; i<=RUNS; i++)); do
        time_val=$($EXEC $t $TEND | grep "Затраченное время выполнения: " | awk '{print $4}' | sed 's/,/./')
        
        total_time=$(awk "BEGIN {print $total_time + $time_val}")
    done
    
    avg_time=$(awk "BEGIN {printf \"%.6f\", $total_time / $RUNS}")
    
    if [ "$t" -eq 1 ]; then
        time_1=$avg_time
        speedup=1.000000
        efficiency=1.000000
    else
        speedup=$(awk "BEGIN {printf \"%.6f\", $time_1 / $avg_time}")
        efficiency=$(awk "BEGIN {printf \"%.6f\", $speedup / $t}")
    fi
    printf "%-10s | %-15s | %-15s | %-15s\n" "$t" "$avg_time" "$speedup" "$efficiency"
done

echo "--------------------------------------------------------------------------------"
echo "Тестирование завершено!"
