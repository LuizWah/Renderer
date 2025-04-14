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





