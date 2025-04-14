## Pipeline de Renderização: O Loop Principal

Esta seção detalha o loop de renderização central implementado em `main.cpp`. Ele descreve os passos envolvidos na transformação de dados de um modelo 3D em uma imagem 2D exibida na tela.

O processo de renderização itera sobre cada face (triângulo) do modelo 3D carregado e executa as seguintes operações chave:

1.  **Processamento de Vértices (Shader.vertex()):**
    * Para cada um dos três vértices da face atual (`model->face(i)`), a função `shader.vertex(i, j)` é chamada. Este é o primeiro estágio do pipeline de gráficos programável.
    * **Dados de Entrada:** A função recebe o índice da face (`iface`) e o índice do vértice dentro dessa face (`nthvert`). Ela acessa as posições dos vértices do modelo (`model->vert(iface, nthvert)`), as coordenadas UV (`model->uv(iface, nthvert)`) e as normais dos vértices (`model->normal(iface, nthvert)`).
    * **Armazenamento de Coordenadas UV:** As coordenadas UV do vértice atual são armazenadas na matriz `varying_uv` do objeto `Shader`. Estas serão interpoladas através da face no shader de fragmentos.
    * **Transformação das Normais:** A normal do vértice é transformada do espaço do modelo para o espaço mundial e, em seguida, para a orientação do espaço de clipe. Isso envolve a multiplicação da normal pela transposta inversa da matriz Model-View-Projection combinada. O resultado é armazenado na matriz `varying_nm`.
    * **Transformação dos Vértices:** A posição do vértice é transformada através do pipeline de gráficos.
        * Primeiro, a posição do vértice 3D é incorporada em uma coordenada homogênea 4D (`embed`).
        * Em seguida, este vértice 4D é multiplicado pela matriz **Model-View-Projection (MVP)** combinada (`Projected`). A `viewMatrix` (transformação da câmera) e a `MatrixProjection` (projeção perspectiva) são pré-calculadas na função `main` e armazenadas no objeto `Shader`.
        * As coordenadas resultantes no espaço de clipe 4D (`gl_Vertex`) são armazenadas na matriz `varying_tri` e retornadas pelo shader de vértices.

2.  **Transformação de Mundo para Tela (WorldToScreen()):**
    * A função `WorldToScreen` recebe as coordenadas no espaço de clipe (`coords`) dos três vértices de um triângulo e as transforma em coordenadas de tela.
    * **Transformação de Visualização:** As coordenadas no espaço de clipe são primeiro multiplicadas pela `viewMatrix` para obter as coordenadas no espaço da câmera (`TranlatedVertices`). A coordenada Z desses vértices no espaço da câmera (`OGdepth`) é armazenada para o buffer de profundidade.
    * **Transformação de Projeção:** As coordenadas no espaço da câmera são então multiplicadas pela `MatrixProjection` para obter as coordenadas projetadas (`ProjectedVertices`).
    * **Normalização e Escala:** As coordenadas projetadas são normalizadas (divididas pelo componente W implicitamente após a divisão perspectiva, que acontece conceitualmente após esta função) e, em seguida, escalonadas para as dimensões da tela (largura e altura). As coordenadas `X` e `Y` são deslocadas em 1 e então multiplicadas pela metade da largura e altura da tela, respectivamente, para mapear o intervalo normalizado \([-1, 1]\) para o intervalo da tela \([0, width]\) e \([0, height]\).

3.  **Rasterização de Triângulos e Processamento de Fragmentos (triangle\_filled\_depth()):**
    * A função `triangle_filled_depth` recebe os vértices 2D no espaço da tela (`vertices`), o buffer de profundidade (`Depthbuffer.buffer`), uma variável de cor, o objeto `TGAImage` para desenhar, o objeto `Shader` e as profundidades originais dos vértices (`OGdepth`).
    * **Cálculo da Caixa Delimitadora:** Primeiro, ela determina a caixa delimitadora (coordenadas X e Y mínimas e máximas) do triângulo projetado.
    * **Iteração de Pixels:** Em seguida, ela itera sobre cada pixel dentro desta caixa delimitadora.
    * **Coordenadas Baricêntricas:** Para cada pixel, ela calcula as coordenadas baricêntricas em relação aos vértices do triângulo usando a função `barycentric` (definida em `math.h`, não mostrada nos trechos fornecidos). As coordenadas baricêntricas representam o peso de cada vértice em um determinado ponto dentro ou fora do triângulo.
    * **Teste de Dentro do Triângulo:** Ela verifica se o pixel atual está dentro do triângulo, verificando se todas as coordenadas baricêntricas são não negativas (ou com base no sinal da função de aresta).
    * **Interpolação de Profundidade:** Se o pixel estiver dentro do triângulo, sua profundidade (`point.z`) é interpolada usando as coordenadas baricêntricas e as profundidades originais dos vértices do triângulo (`OGdepth`).
    * **Teste de Profundidade:** A profundidade interpolada é comparada com o valor armazenado no buffer de profundidade (`Zbuffer[x][y]`) nas coordenadas do pixel atual. Se a profundidade do pixel atual for maior (mais perto da câmera), significa que este pixel está na frente de qualquer geometria desenhada anteriormente nesta localização da tela.
    * **Shading de Fragmentos (shader.fragment()):** Se o teste de profundidade passar, a função `shader.fragment(barycentrics, color)` é chamada. Este é o segundo estágio do pipeline de gráficos programável.
        * **Dados de Entrada:** A função recebe as coordenadas baricêntricas do fragmento atual.
        * **Normal e UV Interpolados:** Dentro do shader de fragmentos, as normais dos vértices e as coordenadas UV (armazenadas em `varying_nm` e `varying_uv`, respectivamente) são interpoladas através do triângulo usando as coordenadas baricêntricas.
        * **Cálculo de Iluminação:** A normal interpolada (`bn`) é usada para calcular a iluminação difusa, pegando o produto escalar com a direção da luz normalizada (`light_dir`). O `std::max(0.f, ...)` garante que a iluminação seja aplicada apenas quando a superfície estiver voltada para a luz.
        * **Texturização:** As coordenadas UV interpoladas (`uv`) são usadas para amostrar a textura do modelo (`model->diffuse(uv)`).
        * **Saída de Cor:** A cor final do fragmento é determinada multiplicando a cor da textura pelo fator de iluminação difusa.
        * **Descarte de Fragmentos:** O shader de fragmentos pode opcionalmente retornar `true` para descartar o fragmento (por exemplo, para implementar teste alfa), mas neste caso, ele sempre retorna `false`.
    * **Atualização do Buffer de Profundidade e da Imagem:** Se o fragmento não for descartado, o buffer de profundidade nas coordenadas do pixel atual é atualizado com a profundidade interpolada, e o pixel na imagem `render` é definido com a cor calculada.

4.  **Saída da Imagem:**
    * Após processar todas as faces do modelo, a imagem renderizada armazenada no objeto `render` é escrita em um arquivo TGA chamado "wwwwwwwwwwwwwwwwww.tga", tem esse nome porque tinham muitos arquivos e tava ficando dificil de encontrat.

Este processo passo a passo, executado para cada triângulo no modelo 3D, forma a base do pipeline de renderização 3D implementado neste código C++. O uso de um shader de vértices e um shader de fragmentos permite a personalização dos estágios de transformação e shading, possibilitando vários efeitos de renderização. O buffer de profundidade garante que objetos mais próximos ocluam os mais distantes, criando uma sensação de profundidade 3D na imagem final que pode parecer algo assim: 

![imagem boa](https://github.com/user-attachments/assets/fc3aa4e0-8cc2-429f-b70b-2c39c061aea4)





## Funções Matemáticas (`geometry.h`)

Esta seção descreve as funções matemáticas e a estrutura definidas no arquivo `geometry.h`, que são essenciais para o pipeline de renderização 3D.

1.  **`barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f P)`:**
    * **Propósito:** Calcula as coordenadas baricêntricas de um ponto `P` com relação a um triângulo definido pelos vértices `a`, `b` e `c`.
    * **Coordenadas Baricêntricas:** Estas coordenadas $(\alpha, \beta, \gamma)$ (representadas aqui como `u`, `v`, `w`) expressam qualquer ponto dentro ou no plano do triângulo como uma soma ponderada de seus vértices:
      $$P = \alpha a + \beta b + \gamma c$$
      onde $\alpha + \beta + \gamma = 1$.
    * **Uso em Renderização:** As coordenadas baricêntricas são cruciais para interpolar atributos de vértice (como coordenadas UV, normais, cores e profundidade) através da superfície de um triângulo durante a rasterização. O valor interpolado em um ponto $P$ dentro do triângulo é dado por:
      $$atributo_P = \alpha \cdot atributo_a + \beta \cdot atributo_b + \gamma \cdot atributo_c$$
    * **Implementação:** A função usa uma fórmula baseada nas áreas dos sub-triângulos formados pelo ponto $P$ e os vértices do triângulo original. O `Vec3f` retornado contém as coordenadas baricêntricas $(u, v, w)$ correspondentes aos pesos dos vértices $a$, $b$ e $c$, respectivamente.
    * **Valor de Retorno:**
        * Um `Vec3f` contendo as coordenadas baricêntricas $(u, v, w)$ se o ponto $P$ estiver dentro ou no plano do triângulo.
        * Um `Vec3f` com componentes $(-1, -1, -1)$ se o denominador no cálculo estiver próximo de zero, indicando um triângulo degenerado ou um problema no cálculo.

2.  **`struct matrix`:**
    * **Propósito:** Define uma estrutura de matriz 4x4.
    * **Membros:** Um array 2D `float m[4][4]` para armazenar os 16 elementos da matriz.
    * **Uso:** Estas matrizes são fundamentais para realizar transformações lineares em gráficos 3D, incluindo:
        * **Transformações de Modelo:** Posicionar, rotacionar e escalar objetos no mundo.
        * **Transformações de Visualização:** Mover e orientar a câmera virtual.
        * **Transformações de Projeção:** Projetar a cena 3D em um plano 2D (por exemplo, projeções perspectiva e ortográfica).

3.  **`matrix matrix_matrix_mult(matrix& m1, matrix& m2, matrix& result)`:**
    * **Propósito:** Realiza a multiplicação de duas matrizes 4x4, `m1` e `m2`, e armazena o resultado na matriz `result`.
    * **Regra de Multiplicação de Matrizes:** O elemento na linha $i$ e coluna $j$ da matriz resultante é calculado como o produto escalar da $i$-ésima linha da primeira matriz e a $j$-ésima coluna da segunda matriz:
      $$result_{ij} = \sum_{k=0}^{3} m1_{ik} \cdot m2_{kj}$$
    * **Implementação:** A função itera pelas linhas e colunas da matriz resultante e calcula cada elemento de acordo com a regra de multiplicação de matrizes. Note que a implementação fornecida tem um erro: ela usa adição (`+`) em vez de multiplicação (`*`) no loop interno. A implementação correta seria:
      ```c++
      matrix matrix_matrix_mult(matrix& m1, matrix& m2, matrix& result){
          for(int i = 0; i < 4; i++){
              for(int j = 0; j < 4; j++){
                  result.m[i][j] = 0; // Inicializa o elemento do resultado
                  for(int k = 0; k < 4; k++){
                      result.m[i][j] += m1.m[i][k] * m2.m[k][j];
                  }
              }
          }
          return result;
      }
      ```

4.  **`matrix transpose(matrix m , matrix &transposed_matrix)`:**
    * **Propósito:** Calcula a transposta de uma matriz 4x4 dada `m` e a armazena em `transposed_matrix`.
    * **Operação de Transposição:** A transposta de uma matriz é obtida trocando suas linhas por colunas. Se $A$ é uma matriz com elemento $a_{ij}$ na linha $i$ e coluna $j$, então sua transposta $A^T$ tem elemento $a_{ji}$ na linha $i$ e coluna $j$.
    * **Implementação:** A função itera pelos elementos da matriz de entrada e atribui `m.m[j][i]` a `transposed_matrix.m[i][j]`.

5.  **`float det3x3(float (*matrix)[3])`:**
    * **Propósito:** Calcula o determinante de uma matriz 3x3.
    * **Determinante:** Um valor escalar que pode ser computado a partir dos elementos de uma matriz quadrada. Ele tem várias interpretações importantes, incluindo representar o fator de escala de uma transformação linear.
    * **Implementação:** A função usa a fórmula padrão para calcular o determinante de uma matriz 3x3.

6.  **`matrix minor_matrices(matrix m, matrix &minors)`:**
    * **Propósito:** Calcula a matriz de menores para uma matriz 4x4 dada `m` e a armazena em `minors`.
    * **Menor:** O menor $M_{ij}$ de uma matriz é o determinante da submatriz formada pela remoção da $i$-ésima linha e $j$-ésima coluna.
    * **Implementação:** A função itera por cada elemento da matriz de entrada `m`. Para cada elemento na linha $k$ e coluna $l$, ela cria uma submatriz 3x3 excluindo a $k$-ésima linha e a $l$-ésima coluna de `m`. Em seguida, ela calcula o determinante desta submatriz 3x3 usando `det3x3` e o armazena em `minors.m[k][l]`, aplicando um sinal baseado na posição $(-1)^k$. Note que o sinal correto para obter o cofator é $(-1)^{k+l}$.

7.  **`matrix Invert4x4(matrix m, matrix &inverted)`:**
    * **Propósito:** Calcula a inversa de uma matriz 4x4 `m` e a armazena em `inverted`.
    * **Inversa de Matriz:** A inversa de uma matriz quadrada $A$, denotada como $A^{-1}$, é uma matriz tal que, quando multiplicada por $A$, resulta na matriz identidade $I$ ($A \cdot A^{-1} = A^{-1} \cdot A = I$). Uma matriz é invertível se e somente se seu determinante for diferente de zero.
    * **Implementação:** A função calcula a matriz de menores usando `minor_matrices`. Então, ela calcula o determinante da matriz original `m` usando a primeira linha e os menores correspondentes (note: o sinal deve alternar). Finalmente, ela транспонирует a matriz de menores (obtendo efetivamente a matriz adjunta) e divide cada elemento pelo determinante para obter a matriz inversa. O sinal para os menores deve ser aplicado ao calcular a matriz de cofatores (antes de transpor).

8.  **`matrix invert_transpose(matrix m, matrix &inverted_transposed)`:**
    * **Propósito:** Calcula a transposta da inversa de uma matriz 4x4 `m` e a armazena em `inverted_transposed`.
    * **Transposta da Inversa:** A transposta da inversa de uma matriz $A$ é $(A^{-1})^T$, que é igual a $(A^T)^{-1}$.
    * **Uso em Renderização:** A transposta da inversa da matriz model-view é frequentemente usada para transformar as normais dos vértices corretamente. Isso ocorre porque as normais são vetores e se transformam de maneira diferente dos pontos quando há escalonamento não uniforme.
    * **Implementação:** A função primeiro calcula a inversa da matriz de entrada `m` usando `Invert4x4` e, em seguida, транспонирует a matriz inversa resultante usando a função `transpose`.

9.  **`void matrixVec4fMulti(Vec4f &i, Vec4f &o, matrix &m)`:**
    * **Propósito:** Multiplica um vetor 4D `i` por uma matriz 4x4 `m` e armazena o vetor 4D resultante em `o`.
    * **Coordenadas Homogêneas:** Os vetores 4D provavelmente estão em coordenadas homogêneas, onde o quarto componente (w) é usado para divisão perspectiva e para representar translações.
    * **Divisão Perspectiva:** Após a multiplicação, se o componente w do vetor de saída `o[3]` não for zero, os componentes x, y e z são divididos por ele para realizar a divisão perspectiva, projetando o ponto 3D no plano de projeção. Note que o código usa incorretamente `i[2]` para inicializar `o[3]` e depois divide por `o[2]`. A implementação correta deveria usar `i[3]` e dividir por `o[3]`:
      ```c++
      void matrixVec4fMulti(Vec4f &i, Vec4f &o, matrix &m) {
          o[0] = m.m[0][0] * i[0] + m.m[1][0] * i[1] + m.m[2][0] * i[2] + m.m[3][0] * i[3];
          o[1] = m.m[0][1] * i[0] + m.m[1][1] * i[1] + m.m[2][1] * i[2] + m.m[3][1] * i[3];
          o[2] = m.m[0][2] * i[0] + m.m[1][2] * i[1] + m.m[2][2] * i[2] + m.m[3][2] * i[3];
          o[3] = m.m[0][3] * i[0] + m.m[1][3] * i[1] + m.m[2][3] * i[2] + m.m[3][3] * i[3];

          if (o[3] != 0.0f) {
              o[0] /= o[3];
              o[1] /= o[3];
              o[2] /= o[3];
          }
      }
      ```

10. **`void matrixVectorMulti(Vec3f &i, Vec3f &o, matrix &m)`:**
    * **Propósito:** Multiplica um vetor 3D `i` por uma matriz 4x4 `m` e armazena o vetor 3D resultante em `o`.
    * **Extensão Homogênea:** O vetor 3D de entrada `i` é implicitamente tratado como um vetor 4D $(i.x, i.y, i.z, 1.0)$ antes da multiplicação, permitindo que ele seja afetado pelo componente de translação da matriz.
    * **Divisão Perspectiva:** Semelhante a `matrixVec4fMulti`, se o componente w resultante não for zero, os componentes x, y e z são divididos por ele.

11. **`Vec3f cross(const Vec3f &a, const Vec3f &b)`:**
    * **Propósito:** Calcula o produto vetorial de dois vetores 3D `a` e `b`.
    * **Produto Vetorial:** O produto vetorial de dois vetores resulta em um novo vetor que é perpendicular a ambos os vetores originais. Sua direção é dada pela regra da mão direita, e sua magnitude é igual à área do paralelogramo formado pelos dois vetores $||a \times b|| = ||a|| \cdot ||b|| \cdot \sin(\theta)$.
    * **Uso em Renderização:** O produto vetorial é usado para vários cálculos, como:
        * Encontrar o vetor normal de um polígono.
        * Determinar a orientação de superfícies.
        * Calcular torque e outras quantidades relacionadas à física.
    * **Implementação:** A função implementa a fórmula padrão para o produto vetorial de dois vetores 3D.

12. **`Vec3f Vec4fToVec3f_aux(Vec4f vec4, Vec3f &new_vec3 )`:**
    * **Propósito:** Converte um vetor 4D `vec4` para um vetor 3D `new_vec3`, simplesmente pegando os três primeiros componentes. O quarto componente (w) é descartado.

13. **`Vec3f* Vec4fToVec3f(Vec4f vec4[], Vec3f vec3[])`:**
    * **Propósito:** Converte um array de vetores 4D `vec4` para um array de vetores 3D `vec3`.
    * **Implementação:** Ela itera pelo array de entrada e chama `Vec4fToVec3f_aux` para cada vetor.

Estas funções matemáticas e a estrutura `matrix` fornecem os blocos de construção fundamentais para realizar as transformações geométricas e os cálculos necessários no pipeline de renderização 3D. Entender seu propósito e implementação é crucial para compreender como as cenas 3D são projetadas e renderizadas em uma tela 2D.

## Estruturas e Classes Utilizadas no Renderer

Esta seção detalha as estruturas (`struct`) e classes (`class`) definidas e utilizadas dentro do código C++ do renderer 3D fornecido.

**1. `struct zbuffer` (Definida em `gl.h`)**

* **Propósito:** Representa um buffer de profundidade (ou Z-buffer), que é usado para determinar a visibilidade de superfícies na renderização 3D. Ele armazena a profundidade do pixel mais próximo renderizado até o momento em cada coordenada da tela.
* **Membros:**
    * `float** buffer;`: Um array 2D de números de ponto flutuante. `buffer[x][y]` armazena o valor de profundidade para o pixel nas coordenadas da tela $(x, y)$.
* **Construtor `zbuffer(int width, int height)`:**
    * Aloca um array 2D de tamanho `width` x `height` para armazenar os valores de profundidade.
    * Inicializa todos os valores de profundidade no buffer com `std::numeric_limits<float>::lowest()`. Isso garante que o primeiro fragmento renderizado em um pixel seja sempre considerado mais próximo do que o valor inicial.
* **Uso:** Durante a rasterização de triângulos, a profundidade de cada fragmento é calculada e comparada com o valor no buffer de profundidade nas coordenadas da tela do fragmento. Se a profundidade do fragmento estiver mais próxima da câmera (valor de profundidade maior nesta implementação), ela sobrescreve o valor de profundidade existente e sua cor é escrita na imagem renderizada. Este processo garante que objetos mais próximos do visualizador ocluam objetos mais distantes.

**2. `struct IShader` (Definida em `gl.h`)**

* **Propósito:** Define uma interface (classe base abstrata) para shaders. Shaders são programas que rodam na unidade de processamento gráfico (GPU) e são responsáveis por transformar vértices (vertex shader) e determinar a cor de fragmentos (pixels) durante a rasterização (fragment shader).
* **Membros:**
    * `virtual Vec4f vertex(int iface, int nthvert) = 0;`: Uma função virtual pura que representa o estágio do vertex shader. Ela recebe o índice da face (`iface`) e o índice do vértice dentro dessa face (`nthvert`) como entrada e espera-se que retorne a posição transformada do vértice no espaço de clipe (como um `Vec4f`). O `= 0` indica que implementações concretas de shader devem fornecer sua própria implementação desta função.
    * `virtual bool fragment(Vec3f bar, TGAColor &color) = 0;`: Uma função virtual pura que representa o estágio do fragment shader. Ela recebe as coordenadas baricêntricas (`bar`) do fragmento como entrada e uma referência a um objeto `TGAColor` (`color`). Espera-se que o shader calcule a cor final do fragmento e a armazene no objeto `color`. Ele também pode retornar um valor `bool` (embora não seja usado na implementação `Shader` fornecida) para indicar se o fragmento deve ser descartado.
* **Uso:** A interface `IShader` permite que diferentes algoritmos de shading sejam implementados criando classes que herdam de `IShader` e sobrescrevem as funções `vertex` e `fragment`. Isso fornece flexibilidade em como o modelo é renderizado.

**3. `struct matrix` (Definida em `geometry.h`)**

* **Propósito:** Representa uma matriz 4x4, que é fundamental para realizar transformações lineares em gráficos 3D.
* **Membros:**
    * `float m[4][4] = {0};`: Um array 2D de números de ponto flutuante para armazenar os 16 elementos da matriz. Inicializado com zero.
* **Uso:** As transformações de matriz são usadas para várias operações no pipeline de renderização, incluindo:
    * **Transformação de Modelo:** Posicionar, rotacionar e escalar objetos na cena.
    * **Transformação de Visualização:** Transformar o mundo de forma que a câmera esteja na origem, olhando para o eixo Z negativo.
    * **Transformação de Projeção:** Projetar a cena 3D em um plano 2D (perspectiva ou ortográfica).

**4. `class Model` (Definida em `model.h`)**

* **Propósito:** Representa um modelo 3D carregado de um arquivo (provavelmente um arquivo OBJ com base no uso do construtor). Ele armazena a geometria do modelo (vértices, faces, normais, coordenadas UV) e as texturas associadas.
* **Membros Privados:**
    * `std::vector<Vec3f> verts_;`: Um vetor de objetos `Vec3f` armazenando as coordenadas 3D dos vértices do modelo.
    * `std::vector<std::vector<Vec3i> > faces_;`: Um vetor de vetores de `Vec3i`. Cada vetor interno representa uma face (triângulo) e armazena três objetos `Vec3i`. Cada `Vec3i` provavelmente contém os índices do vértice, coordenada UV e normal para cada canto da face.
    * `std::vector<Vec3f> norms_;`: Um vetor de objetos `Vec3f` armazenando os vetores normais para cada vértice.
    * `std::vector<Vec2f> uv_;`: Um vetor de objetos `Vec2f` armazenando as coordenadas UV 2D para mapeamento de textura.
    * `TGAImage diffusemap_;`: Um objeto `TGAImage` armazenando a textura difusa do modelo (a cor base).
    * `TGAImage normalmap_;`: Um objeto `TGAImage` armazenando o mapa de normais, usado para adicionar detalhes à superfície.
    * `TGAImage specularmap_;`: Um objeto `TGAImage` armazenando o mapa especular, que controla a intensidade dos realces.
    * `void load_texture(std::string filename, const char *suffix, TGAImage &img);`: Uma função auxiliar privada para carregar imagens de textura de arquivos com base em um nome de arquivo e sufixo.
* **Membros Públicos:**
    * `Model(const char *filename);`: O construtor da classe `Model`. Ele provavelmente recebe o nome do arquivo do modelo como entrada e carrega a geometria e as texturas.
    * `~Model();`: O destrutor da classe `Model`, responsável por liberar quaisquer recursos alocados.
    * `int nverts();`: Retorna o número de vértices no modelo.
    * `int nfaces();`: Retorna o número de faces (triângulos) no modelo.
    * `Vec3f normal(int iface, int nthvert);`: Retorna o vetor normal do `nthvert`-ésimo vértice da `iface`-ésima face.
    * `Vec3f normal(Vec2f uv);`: Retorna o vetor normal nas coordenadas UV fornecidas (provavelmente amostrado do mapa de normais).
    * `Vec3f vert(int i);`: Retorna as coordenadas 3D do `i`-ésimo vértice.
    * `Vec3f vert(int iface, int nthvert);`: Retorna as coordenadas 3D do `nthvert`-ésimo vértice da `iface`-ésima face.
    * `Vec2f uv(int iface, int nthvert);`: Retorna as coordenadas UV do `nthvert`-ésimo vértice da `iface`-ésima face.
    * `TGAColor diffuse(Vec2f uv);`: Retorna a cor difusa nas coordenadas UV fornecidas (amostrada da textura difusa).
    * `float specular(Vec2f uv);`: Retorna a intensidade especular nas coordenadas UV fornecidas (amostrada do mapa especular).
    * `std::vector<int> face(int idx);`: Retorna um vetor de índices representando os vértices da `idx`-ésima face. Com base no membro privado `faces_`, isso provavelmente retorna os índices dos vértices (e possivelmente os índices de UV e normal compactados juntos).
* **Uso:** A classe `Model` encapsula os dados e a funcionalidade necessários para representar e acessar a geometria e a aparência de um objeto 3D. O loop de renderização em `main.cpp` interage com um objeto `Model` para recuperar as posições dos vértices, normais, coordenadas UV e informações de textura para cada face a ser renderizada.

**5. `struct Shader` (Definida em `main.cpp`)**

* **Propósito:** Uma implementação concreta da interface `IShader`, definindo um pipeline de shading específico para renderizar o modelo.
* **Membros:**
    * `mat<2,3,float> varying_uv;`: Uma matriz para armazenar as coordenadas UV interpoladas para os três vértices do triângulo atual. O prefixo `varying_` sugere que esses valores variarão pela superfície do triângulo e devem ser interpolados para cada fragmento.
    * `mat<3,3,float> varying_nm;`: Uma matriz para armazenar os vetores normais interpolados para os três vértices do triângulo atual (após a transformação).
    * `mat<4,3,float> varying_tri;`: Uma matriz para armazenar as coordenadas no espaço de clipe (`gl_Vertex`) dos três vértices do triângulo atual.
    * `matrix MatrixProjection;`: Armazena a matriz de projeção, que transforma as coordenadas 3D do mundo em espaço de clipe 2D.
    * `matrix viewMatrix;`: Armazena a matriz de visualização (transformação da câmera), que transforma as coordenadas do mundo em espaço da câmera.
* **Funções Virtuais Sobrescritas (de `IShader`):**
    * `virtual Vec4f vertex(int iface, int nthvert)`: Implementa o estágio do vertex shader. Ele recupera os dados do vértice do `model`, transforma a posição do vértice pela matriz Model-View-Projection combinada e armazena as coordenadas UV e as normais transformadas nas matrizes `varying_uv` e `varying_nm`, respectivamente. Ele retorna a posição transformada do vértice no espaço de clipe (`gl_Vertex`).
    * `virtual bool fragment(Vec3f bar, TGAColor &color)`: Implementa o estágio do fragment shader. Ele recebe as coordenadas baricêntricas (`bar`) do fragmento, interpola as normais e as coordenadas UV usando esses pesos baricêntricos, calcula a iluminação difusa com base na normal interpolada e na direção da luz, amostra a textura difusa usando as coordenadas UV interpoladas e define a cor do fragmento.
* **Uso:** Uma instância da struct `Shader` é criada em `main.cpp` e seus métodos `vertex` e `fragment` são chamados para cada vértice e fragmento do modelo que está sendo renderizado. `MatrixProjection` e `viewMatrix` são definidas em `main.cpp` e usadas dentro do vertex shader.

Essas estruturas e classes trabalham juntas para definir os dados, as transformações e os processos de shading envolvidos na renderização do modelo 3D em uma imagem 2D. A classe `Model` fornece os dados 3D, a classe `Shader` define como esses dados são processados e coloridos, a struct `matrix` é usada para transformações geométricas e a struct `zbuffer` ajuda a resolver a visibilidade. A interface `IShader` permite que diferentes algoritmos de shading sejam conectados ao pipeline de renderização.

## Shaders: O `struct Shader` e seu Funcionamento

Esta seção detalha o `struct Shader` definido no arquivo `main.cpp` e explica como ele funciona dentro do pipeline de renderização. Este `struct` implementa a interface `IShader`, fornecendo a lógica específica para os estágios de vertex e fragment shader.

O `Shader` neste código realiza um shading básico com iluminação difusa e utiliza informações de textura do modelo.

**Membros do `struct Shader`:**

* **`mat<2,3,float> varying_uv;`**: Uma matriz 2x3 que armazena as coordenadas UV para os três vértices do triângulo atualmente sendo processado. As coordenadas UV originais de cada vértice são atribuídas a uma coluna desta matriz no vertex shader. Durante a rasterização, essas coordenadas são interpoladas para cada fragmento usando as coordenadas baricêntricas. O prefixo `varying_` indica que esses valores variam pela superfície do triângulo.

* **`mat<3,3,float> varying_nm;`**: Uma matriz 3x3 que armazena os vetores normais (transformados) para os três vértices do triângulo atual. Similarmente às coordenadas UV, as normais transformadas de cada vértice são armazenadas em uma coluna e serão interpoladas para cada fragmento.

* **`mat<4,3,float> varying_tri;`**: Uma matriz 4x3 que armazena as coordenadas homogêneas (Vec4f) dos três vértices do triângulo após a transformação para o espaço de clipe (`gl_Vertex`). Embora este membro esteja presente, no código fornecido, a função `WorldToScreen` opera diretamente nos resultados do `shader.vertex`, e `varying_tri` não parece ser diretamente utilizada na rasterização ou no fragment shader.

* **`matrix MatrixProjection;`**: Uma matriz 4x4 que armazena a matriz de projeção perspectiva. Esta matriz é calculada na função `main` com base no campo de visão (FOV), proporção da tela (AspectRatio), plano de corte próximo (znear) e plano de corte distante (zfar). Ela é usada no vertex shader para projetar os vértices 3D no espaço de clipe.

* **`matrix viewMatrix;`**: Uma matriz 4x4 que armazena a matriz de visualização (ou câmera). Esta matriz é calculada na função `main` com base na posição da câmera (`cameraPos`), o ponto para onde a câmera está olhando (`cameraTarget`) e o vetor "para cima" da câmera (`cameraUp`). Ela transforma as coordenadas do mundo para as coordenadas da câmera.

**Estágios do Shader:**

O `struct Shader` implementa os dois estágios programáveis do pipeline de gráficos: o vertex shader (`vertex`) e o fragment shader (`fragment`).

1.  **Vertex Shader (`virtual Vec4f vertex(int iface, int nthvert)`)**:
    * Esta função é executada uma vez para cada vértice de cada triângulo do modelo.
    * **Entrada:** Recebe o índice da face (`iface`) e o índice do vértice dentro dessa face (`nthvert`).
    * **Processamento:**
        * **Busca de Atributos:** Recupera as coordenadas UV do vértice (`model->uv(iface, nthvert)`) e as armazena na coluna `nthvert` da matriz `varying_uv`.
        * **Transformação da Normal:** Transforma a normal do vértice (`model->normal(iface, nthvert)`) do espaço do modelo para uma orientação adequada no espaço de clipe. Isso envolve a multiplicação pela transposta inversa da matriz Model-View-Projection (MVP). O resultado é armazenado na coluna `nthvert` da matriz `varying_nm`.
        * **Transformação do Vértice:** Transforma a posição do vértice (`model->vert(iface, nthvert)`) do espaço do modelo para o espaço de clipe. Isso é feito multiplicando as coordenadas homogêneas do vértice (criadas com `embed`) pela matriz Model-View-Projection (`Projected`), que é a multiplicação da `MatrixProjection` pela `viewMatrix`.
        * **Armazenamento da Posição Projetada:** A posição do vértice transformada para o espaço de clipe (`gl_Vertex`) é armazenada na coluna `nthvert` da matriz `varying_tri`.
    * **Saída:** Retorna as coordenadas do vértice transformadas para o espaço de clipe (`gl_Vertex`).

2.  **Fragment Shader (`virtual bool fragment(Vec3f bar, TGAColor &color)`)**:
    * Esta função é executada para cada fragmento (potencial pixel) gerado durante a rasterização de um triângulo.
    * **Entrada:** Recebe as coordenadas baricêntricas (`bar`) do fragmento.
    * **Processamento:**
        * **Interpolação de Atributos:** Interpola o vetor normal (`bn`) e as coordenadas UV (`uv`) para o fragmento atual usando as coordenadas baricêntricas e os valores armazenados nas matrizes `varying_nm` e `varying_uv` (respectivamente). A normal interpolada é então normalizada.
        * **Cálculo da Iluminação Difusa:** Calcula a intensidade da iluminação difusa tomando o produto escalar entre a normal interpolada (`bn`) e a direção da luz normalizada (`light_dir`). A função `std::max(0.f, ...)` garante que a iluminação seja apenas positiva (quando a superfície está voltada para a luz).
        * **Texturização:** Amostra a cor difusa da textura do modelo (`model->diffuse(uv)`) usando as coordenadas UV interpoladas.
        * **Cálculo da Cor Final:** A cor final do fragmento (`color`) é determinada multiplicando a cor difusa da textura pela intensidade da iluminação difusa.
    * **Saída:** Retorna um valor booleano (`false` neste caso), que pode ser usado para descartar o fragmento (por exemplo, para efeitos de alpha testing), mas aqui sempre indica que o fragmento deve ser processado.

**Fluxo de Dados:**

O vertex shader processa cada vértice do modelo, transformando sua posição e passando dados (como UVs e normais transformadas) para o estágio de rasterização através das variáveis `varying_`. O rasterizador então interpola esses valores `varying_` através da superfície de cada triângulo, e os valores interpolados (juntamente com as coordenadas baricêntricas) são passados para o fragment shader. O fragment shader usa esses dados interpolados para calcular a cor final de cada fragmento, que é então escrita no framebuffer (a imagem final).

Em resumo, o `struct Shader` define a lógica específica de como os vértices são transformados e como a cor de cada pixel na imagem final é determinada, implementando um modelo de iluminação difusa básica com suporte a texturas.

