# Sistema de irrigação automático para jardinagem

Esse foi o projeto desenvolvido e apresentado na disciplina PCS3858 - Laboratório de Sistemas Embarcados(2023). Trata-se de uma solução para irrigação de várias plantas com perfis de irrigação distintos simultaneamente.  

Esse repositório contém o código do embarcado e do aplicativo que interage com aquele para configurar credenciais de Wi-Fi, perfis de irrigação e acionamento dos controles.

Leonardo Martins Pires NUSP 11262091

Marcio Akira Imanishi de Moraes NUSP 11262021

## Embarcado

Para compilar código do embarcado em uma placa ESP32, é necessário usar a extensão Platform.io no VSCode. Nela, o projeto vai ser reconhecido pela presença do arquivo .ini, que determina configurações de build.

## Celular

Para rodar o aplicativo em um celular local, é necessário instalar o ambiente de desenvolvimento seguindo o tutorial disponibilizado pelo [React Native](https://reactnative.dev/docs/environment-setup?guide=native&platform=android&os=windows) para o seu respectivo sistema operacional.
Um .apk pode ser gerado seguindo as instruções da resposta da Cecelia Martinez [no seguinte link](https://stackoverflow.com/questions/75516527/how-to-generate-build-apk-for-android-in-react-native-0-71-2).