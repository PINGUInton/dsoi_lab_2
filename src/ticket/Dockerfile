# Используем многостадийную сборку
FROM ubuntu:22.04 AS builder

# Устанавливаем системные зависимости
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    libpq-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем Conan
RUN pip3 install conan

# Настраиваем Conan
RUN conan profile detect --force

# Создаем рабочую директорию
WORKDIR /app

# Копируем файлы проекта
COPY . .

# Создаем директорию для сборки и устанавливаем зависисмости
RUN mkdir -p build && cd build && conan install --build missing ..

# Собираем проект
RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --config Release --target ticket_service

# Финальный образ
FROM ubuntu:22.04

# Устанавливаем runtime зависимости
RUN apt-get update && apt-get install -y \
    libpq5 \
    libssl3 \
    libgcc-s1 \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Создаем пользователя и директорию для приложения
RUN useradd -m appuser
RUN mkdir -p /app && chown -R appuser:appuser /app

# Копируем собранное приложение
COPY --from=builder /app/build/ticket_service /app/
RUN chmod +x /app/ticket_service

# Переключаемся на не-root пользователя
USER appuser
WORKDIR /app

# Экспонируем порт
EXPOSE 8070

# Запуск приложения
CMD ["./ticket_service"]