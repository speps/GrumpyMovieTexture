Shader "Hidden/GrumpyConvertRGB"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _MainCbTex ("Texture", 2D) = "white" {}
        _MainCrTex ("Texture", 2D) = "white" {}
    }
    SubShader
    {
        // No culling or depth
        Cull Off ZWrite Off ZTest Always

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
                float4 vertex : SV_POSITION;
                float2 uv : TEXCOORD0;
            };

            sampler2D _MainTex;
            sampler2D _MainCbTex;
            sampler2D _MainCrTex;

            v2f vert(appdata v)
            {
                v2f o;
                o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
                o.uv = v.uv;
                o.uv.y = 1.0f - o.uv.y;
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

            float4 frag(v2f i) : SV_Target
            {
                float Y = tex2D(_MainTex, i.uv).a;
                float Cb = tex2D(_MainCbTex, i.uv).a;
                float Cr = tex2D(_MainCrTex, i.uv).a;
                return YCbCr2RGB(float4(Y, Cb, Cr, 1.0f));
            }
            ENDCG
        }
    }
}
