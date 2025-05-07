Устанавливаем драйвер mongodb
```bash
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.0.0/mongo-cxx-driver-r4.0.0.tar.gz
tar -xzf mongo-cxx-driver-r4.0.0.tar.gz
cd mongo-cxx-driver-r4.0.0/build
```

и исполняем команды ниже:

```bash
cmake ..                                \
    -DCMAKE_BUILD_TYPE=Release          \
    -DCMAKE_CXX_STANDARD=17
```

```bash
cmake --build .
sudo cmake --build . --target install
```

При комплияции не забываем указывать путь к библиотеке 
```Makefile
-I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib  test_main.cpp -lmongocxx -lbsoncxx
```

Поднять докер:
```bash
docker compose up -d
```

Остановить докер:
```bash
docker compose down
```

Посмотреть процессы докера
```bash
docker compose ps
```


Запуск файла, в котором используем интерфейс mongoDB:
```bash
LD_LIBRARY_PATH=/usr/local/lib ./a.out
```






