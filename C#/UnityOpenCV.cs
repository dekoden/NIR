using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.UI;

public class UnityOpenCV : MonoBehaviour
{
    public Camera camera1;
    public Camera camera2;
    private Texture2D camTex;
    private Color32[] rawColor;
    private int frameIndex = 0;
    private readonly int cameraCalibSamplesCount = 40;
    unsafe void TextureToPointer(Texture2D raw)
    {
        rawColor = raw.GetPixels32();
        Destroy(raw);
    }

    private Texture2D tex;
    private Color32[] pixel32;

    private GCHandle pixelHandle;
    private IntPtr pixelPtr;


    void InitTexture()
    {
        tex = new Texture2D(camTex.width, camTex.height, TextureFormat.ARGB32, false);
        pixel32 = tex.GetPixels32();
        //Pin pixel32 array
        pixelHandle = GCHandle.Alloc(pixel32, GCHandleType.Pinned);
        //Get the pinned address
        pixelPtr = pixelHandle.AddrOfPinnedObject();
    }

    unsafe void PointerToTexture2D()
    {
        //Convert Mat to Texture2D
        processImage(pixelPtr, tex.width, tex.height);
        //Update the Texture2D with array updated in C++
        tex.SetPixels32(pixel32);
        tex.Apply();
    }
    static public Texture2D RenderTexToTex2D(RenderTexture rt)
    {
        // Remember currently active render texture
        RenderTexture currentActiveRT = RenderTexture.active;

        // Set the supplied RenderTexture as the active one
        RenderTexture.active = rt;

        // Create a new Texture2D and read the RenderTexture image into it
        Texture2D tex = new Texture2D(rt.width, rt.height);
        tex.ReadPixels(new Rect(0, 0, tex.width, tex.height), 0, 0);

        // Restorie previously active render texture
        RenderTexture.active = currentActiveRT;
        return tex;
    }

    static public void CopyTexToRender(Texture2D tex, RenderTexture rt)
    {
        RenderTexture.active = rt;
        Graphics.Blit(tex, rt);
    }

    // Start is called before the first frame update
    void Start()
    {
        
    }

    public RenderTexture modifiedRender1;
    public RenderTexture modifiedRender2;
    public RenderTexture disparityMap;

    // Update is called once per frame
    void Update()
    {
        if(frameIndex < cameraCalibSamplesCount)
        {
            Transform t = GameObject.Find("/pattern").GetComponent<Transform>();
            t.rotation = Quaternion.Euler(
                UnityEngine.Random.Range(-30f, 30f),
                UnityEngine.Random.Range(-30f, 30f),
                UnityEngine.Random.Range(-30f, 30f));
            t.position = new Vector3(
                UnityEngine.Random.Range(-0.4f, 0.4f),
                UnityEngine.Random.Range(1.5f, 2.5f),
                UnityEngine.Random.Range(-2.5f, -1f));
        }
        else
        {
            Destroy(GameObject.Find("/pattern"));
        }

        camTex = RenderTexToTex2D(camera1.targetTexture);
        TextureToPointer(camTex);
        unsafe
        {
            fixed (Color32* p1 = rawColor)
            {
                getImages((IntPtr)p1, camTex.width, camTex.height, frameIndex, 1);
            }
        }
        InitTexture();
        PointerToTexture2D();
        if (frameIndex < cameraCalibSamplesCount)
        {
            RenderTexture.active = modifiedRender1;
            Graphics.Blit(tex, modifiedRender1);
        }

        camTex = RenderTexToTex2D(camera2.targetTexture);
        TextureToPointer(camTex);
        unsafe
        {
            fixed (Color32* p1 = rawColor)
            {
                getImages((IntPtr)p1, camTex.width, camTex.height, frameIndex, 2);
            }
        }
        InitTexture();
        PointerToTexture2D();
        if (frameIndex < cameraCalibSamplesCount)
        {
            RenderTexture.active = modifiedRender2;
            Graphics.Blit(tex, modifiedRender2);
        }
        else
        {
            RenderTexture.active = disparityMap;
            Graphics.Blit(tex, disparityMap);
        }

        frameIndex++;
    }

    [DllImport("processingInOpenCV", EntryPoint = "getImages")]
    unsafe static extern int getImages(IntPtr raw, int width, int height, int numOfImg, int numOfCam);

    [DllImport("processingInOpenCV", EntryPoint = "processImage")]
    unsafe static extern void processImage(IntPtr data, int width, int height);
}
