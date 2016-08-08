Shader "VideoPlayer/VideoPlayer"
{
    Properties
    {
        _MainYTex ("Texture", 2D) = "white" {}
        _MainCbTex ("Texture", 2D) = "white" {}
        _MainCrTex ("Texture", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            
            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uvY : TEXCOORD0;
                float2 uvCb : TEXCOORD1;
                float2 uvCr : TEXCOORD2;
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainYTex;
            float4 _MainYTex_ST;
            sampler2D _MainCbTex;
            float4 _MainCbTex_ST;
            sampler2D _MainCrTex;
            float4 _MainCrTex_ST;
            
            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
                o.uvY = TRANSFORM_TEX(v.uv, _MainYTex);
                o.uvCb = TRANSFORM_TEX(v.uv, _MainCbTex);
                o.uvCr = TRANSFORM_TEX(v.uv, _MainCrTex);
                return o;
            }

            float4 YCbCr2RGB(float4 YCbCr)
            {
                const float4 YCbCr2R = float4(1.1643828125, 0, 1.59602734375, -.87078515625);
                const float4 YCbCr2G = float4(1.1643828125, -.39176171875, -.81296875, .52959375);
                const float4 YCbCr2B = float4(1.1643828125, 2.017234375, 0,  -1.081390625);
                return float4(
                    dot(YCbCr2R, YCbCr),
                    dot(YCbCr2G, YCbCr),
                    dot(YCbCr2B, YCbCr),
                    1.0f
                );
            }
            
            fixed4 frag (v2f i) : SV_Target
            {
                float Y = tex2D(_MainYTex, i.uvY).a;
                float Cb = tex2D(_MainCbTex, i.uvCb).a;
                float Cr = tex2D(_MainCrTex, i.uvCr).a;
                return YCbCr2RGB(float4(Y, Cb, Cr, 1.0f));
            }
            ENDCG
        }
    }
}
