# README #

docker build -t cpp_highload
docker run --rm -p 8080:80 -t cpp_highload
docker login stor.highloadcup.ru
docker tag cpp_highload stor.highloadcup.ru/travels/sharp_wasp
docker push stor.highloadcup.ru/travels/sharp_wasp