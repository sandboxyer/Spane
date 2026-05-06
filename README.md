# Terminal Rush
🔧 ```bash install.sh```

🌎 ```darkterminal```

# Terminal Rush

O **Terminal Rush** é um protótipo de jogo 2D top-down desenvolvido em C. Ele utiliza a biblioteca gráfica nativa do Linux (Xlib) para renderizar um mapa baseado em *tiles*, gerenciar colisões e processar a movimentação fluida de um personagem a aproximadamente 60 FPS.

## Funcionalidades
* **Renderização Gráfica Nativa:** Desenho de cenários (árvores, casas, água, caminhos) e sprites usando primitivas geométricas exclusivas da X11.
* **Movimentação e Animação:** Controle do personagem em 4 direções (teclas WASD ou Setas) com animação cíclica de caminhada baseada em frames.
* **Colisão Precisa:** Verificação de "bounding box" (checagem dos 4 cantos do sprite) para impedir que o personagem atravesse obstáculos.
* **Teleporte de Interativo:** Clique com o botão esquerdo do mouse para mover o personagem instantaneamente para qualquer área acessível.
* **Barra de Status:** Interface inferior exibindo coordenadas do jogador em tempo real e os comandos básicos.

---

## Requisitos do Sistema
* Ambiente Linux (ou WSL no Windows com servidor X gráfico configurado).
* Compilador C (como o GCC).
* Biblioteca de desenvolvimento da X11.

Para instalar as dependências gráficas em sistemas baseados em Ubuntu/Debian, você pode utilizar o comando `sudo apt-get install libx11-dev` no seu terminal.

---

## Compilação e Execução

Como este projeto utiliza ferramentas gráficas que não fazem parte da biblioteca padrão da linguagem C, o processo para gerar o jogo exige uma etapa especial de "linkagem" (conexão) com o sistema de janelas do Linux. Siga os passos abaixo para preparar e rodar o jogo:

**Passo 1: Entendendo o comando de compilação**
Abra o terminal do seu sistema operacional e navegue até a pasta onde você salvou o código-fonte do jogo. Para transformar o seu arquivo de texto em um programa real, você usará o compilador GCC. Você deve digitar `gcc` seguido do nome do seu arquivo (por exemplo, `main.c`). Depois, use o parâmetro `-o terminal_rush` para dizer ao compilador que o arquivo final (o jogo executável) deve se chamar "terminal_rush". 

O detalhe mais importante vem no final: você deve obrigatoriamente digitar a flag `-lX11`. É essa pequena instrução que avisa ao compilador para incluir todas as funções de desenho de mapa e janelas da biblioteca gráfica. O seu comando final digitado no terminal será exatamente assim: `gcc main.c -o terminal_rush -lX11`.

**Passo 2: Iniciando a partida**
Aperte Enter após digitar o comando de compilação. Se o terminal pular para a próxima linha sem mostrar nenhuma mensagem de erro, parabéns! A compilação foi um sucesso e o arquivo executável foi criado na sua pasta. 

Para abrir o jogo, basta comandar ao terminal que execute o arquivo que acabou de ser criado na pasta atual. Você faz isso digitando `./terminal_rush` e pressionando Enter. A janela gráfica do jogo se abrirá e você já poderá começar a explorar o mapa.

---

## Controles

| Ação | Comando |
| :--- | :--- |
| **Mover para Cima** | Seta Acima ou W |
| **Mover para Baixo** | Seta Abaixo ou S |
| **Mover para Esquerda**| Seta Esquerda ou A |
| **Mover para Direita** | Seta Direita ou D |
| **Teleporte rápido** | Clique Botão Esquerdo do Mouse |
| **Sair do Jogo** | ESC |

---

## Elementos do Mapa (Tiles)

O mapa é gerado a partir de uma matriz no código. Os identificadores numéricos representam os seguintes elementos:

* **0:** Grama (Área livre)
* **1:** Parede/Limites do Mapa (Obstáculo)
* **2:** Água (Obstáculo)
* **3:** Caminho de terra (Área livre)
* **4:** Árvore (Obstáculo)
* **5:** Casa (Obstáculo)



## Equipe
- João Lucas Mendes Dos Santos
- Augusto Freitas
- Daniel Luiz Massud
- Fernando Antônio 
- Gustavo Cassemiro
- Glauberson
- Caio Catão



