# Chuva de Frevo

Bem-vindo ao **Chuva de Frevo**! Este é um jogo simples onde o jogador controla um turista que precisa capturar objetos que caem do céu, evitando deixá-los cair no chão. O jogo foi desenvolvido em C usando a biblioteca gráfica [Raylib](https://www.raylib.com/).



## 📜 Descrição

No jogo, você controla um turista que se move lateralmente na parte inferior da tela. Sombrinhas caem do topo da tela, e seu objetivo é capturá-los para ganhar pontos. Alguns objetos especiais concedem power-ups que podem ser usados para eliminar todos os objetos na tela. O jogo termina quando você perde todas as suas vidas.

## 💻 Instalação

1. **Clone o repositório:**

   ```bash
   git clone https://github.com/kauan-novello/Jogo-Chuva-de-Frevo.git

2. **Instale a Raylib:**

Siga as instruções no site oficial da  [Raylib](https://www.raylib.com/) para instalar a biblioteca em seu sistema.

3. **Compile o código:**
Navegue até o diretório do projeto e compile usando o GCC:
```bash
gcc main.c -o jogo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```
# 🎮 Como Jogar

##  🏠 Menu Principal

Aperte 1 para Iniciar Jogo

Aperte 2 Instruções

Aperte 3 Ranking de Pontuações

Aperte 4  Sair do Jogo

## Controles

Seta Esquerda ⬅️ : Move o jogador para a esquerda

Seta Direita ➡️: Move o jogador para a direita

Barra de Espaço: Usa um power-up que "apaga" todas as sombrinhas da tela

R: Retorna ao menu anterior ou principal

ENTER: Confirma ações ou avança nos menus

## Durante o Jogo

1. 💯 Capture os objetos que caem para ganhar pontos
2. ❤️ Evite deixar os objetos caírem no chão, ou você perderá vidas (o jogador possui 3 vidas)
3. ⚡ Capture objetos especiais para ganhar power-ups

 ## Game Over

Se você tiver uma pontuação alta, poderá inserir seu nome para salvar no ranking
Pressione ENTER para continuar

## 👤 Membros do grupo

- Raul Vila Nova -> rvnc@cesar.school
- Arthur Borges -> aab4@cesar.school
- Kauan Novello -> kvns@cesar.school
- Tiago Cavalcanti -> tpbc@cesar.school
- Luís Augusto -> lavc@cesar.school
