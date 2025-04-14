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
    * Após processar todas as faces do modelo, a imagem renderizada armazenada no objeto `render` é escrita em um arquivo TGA chamado "wwwwwwwwwwwwwwwwww.tga".

Este processo passo a passo, executado para cada triângulo no modelo 3D, forma a base do pipeline de renderização 3D implementado neste código C++. O uso de um shader de vértices e um shader de fragmentos permite a personalização dos estágios de transformação e shading, possibilitando vários efeitos de renderização. O buffer de profundidade garante que objetos mais próximos ocluam os mais distantes, criando uma sensação de profundidade 3D na imagem final.
