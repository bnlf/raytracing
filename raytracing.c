/**
 *	@file raytracing.c RayTracing: renderiza��o de cenas por raytracing.
 *
 *	@author 
 *			- Maira Noronha
 *			- Thiago Bastos
 *          - Bruno Nunes
 *          - Bruno Costa
 *          - Rodolfo Caldeira
 *
 *	@date
 *			Criado em:			02 de Dezembro de 2002
 *			�ltima Modifica��o:	23 de Outubro de 2007
 *
 *	@version 2.0
 */

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

#include "raytracing.h"
#include "color.h"
#include "algebra.h"


/************************************************************************/
/* Constantes Privadas                                                  */
/************************************************************************/
#define MAX( a, b ) ( ( a > b ) ? a : b )

/** N�mero m�ximo de recurs�es permitidas */
#define MAX_DEPTH	6


Color lastColor;


/************************************************************************/
/* Fun��es Privadas                                                     */
/************************************************************************/
/**
 *	Obt�m uma cor atrav�s do tra�ado de um raio dentro de uma cena.
 *
 *	@param scene Handle para a cena sendo renderizada.
 *	@param eye Posi��o do observador, origem do raio.
 *	@param ray Dire��o do raio.
 *	@param depth Para controle do n�mero m�ximo de recurs�es. Fun��es clientes devem
 *					passar 0 (zero). A cada recurs�o depth � incrementado at�, no m�ximo,
 *					MAX_DEPTH. Quando MAX_DEPTH � atingido, recurs�es s�o ignoradas.
 *
 *	@return Cor resultante do tra�ado do raio.
 */
static Color shade( Scene* scene, Vector eye, Vector ray, Object* object, Vector point,
					     Vector normal, int depth );

/**
 *	Encontra o primeiro objeto interceptado pelo raio originado na posi��o especificada.
 *
 *	@param scene Cena.
 *	@param eye Posi��o do Observador (origem).
 *	@param ray Raio sendo tra�ado (dire��o).
 *	@param object Onde � retornado o objeto resultante. N�o pode ser NULL.
 *	@return Dist�ncia entre 'eye' e a superf�cie do objeto interceptado pelo raio.
 *			DBL_MAX se nenhum objeto � interceptado pelo raio, neste caso
 *				'object' n�o � modificado.
 */
static double getNearestObject( Scene* scene, Vector eye, Vector ray, Object* *object );

/**
 *	Checa se objetos em uma cena impedem a luz de alcan�ar um ponto.
 *
 *	@param scene Cena.
 *	@param point Ponto sendo testado.
 *	@param rayToLight Um raio (dire��o) indo de 'point' at� 'lightLocation'.
 *	@param lightLocation Localiza��o da fonte de luz.
 *	@return Zero se nenhum objeto bloqueia a luz e n�o-zero caso contr�rio.
 */
static double isInShadow( Scene* scene, Vector point, Vector rayToLight, Vector lightLocation );


/************************************************************************/
/* Defini��o das Fun��es Exportadas                                     */
/************************************************************************/

Color rayTrace( Scene* scene, Vector eye, Vector ray, int depth )
{
	if( depth < MAX_DEPTH )
	{
		Object* object;
		double distance;

		Vector point;
		Vector normal;

		depth++;
		/* Calcula o primeiro objeto a ser atingido pelo raio */
		distance = getNearestObject( scene, eye, ray, &object );

		/* Se o raio n�o interceptou nenhum objeto... */
		if( distance == DBL_MAX )
		{
			return sceneGetBackgroundColor( scene, eye, ray );
		}

		/* Calcula o ponto de interse��o do raio com o objeto */
		point = algAdd( eye, algScale( distance, ray ) );

		/* Obt�m o vetor normal ao objeto no ponto de interse��o */
		normal =  algUnit (objNormalAt( object, point ));

		lastColor = shade( scene, eye, ray, object, point, normal, depth );
		return lastColor;
	}
	return lastColor;
}

/************************************************************************/
/* Defini��o das Fun��es Privadas                                       */
/************************************************************************/

static Color shade( Scene* scene, Vector eye, Vector ray, Object* object, Vector point,
					Vector normal, int depth )
{
	Material* material = sceneGetMaterial(scene,objGetMaterial(object));
	Color ambient = sceneGetAmbientLight( scene );

	Color diffuse = materialGetDiffuse( material, objTextureCoordinateAt( object, point ) );	
	Color specular = materialGetSpecular( material );
	double specularExponent = materialGetSpecularExponent( material );

	double reflectionFactor = materialGetReflectionFactor( material );
	double refractedIndex   = materialGetRefractionIndex( material );
	double opacity = materialGetOpacity( material );
	
	/* Variaveis adicionais */
	Light* intensity;
	Color intensitycolor, color , colorReflec , colorAux, colorOpacity;
	Vector pointToLightRay, pointToLightRayReflected ,pointToEyeRay, reflecRay, softlight ;
	double produto , factor , InterceptionOpacity;
	int w;

	/* Verifica quantidade de luzes */
	int LightCount = sceneGetLightCount(scene);
	
	/* Cor Ambiente */
	color = colorMultiplication( diffuse, ambient );
	
	for (w = 0; w < LightCount ; w++)
	{
		InterceptionOpacity = 0;
		intensity = sceneGetLight ( scene, w );
		softlight = lightGetPosition( intensity );
		intensitycolor = lightGetColor( intensity );

		pointToEyeRay = algSub( eye, point );
		pointToEyeRay = algUnit( pointToEyeRay );

		pointToLightRay = algSub( softlight, point );
		pointToLightRay = algUnit( pointToLightRay );

		/* Aplicar Efeito de Transparencia e Refracao*/
		if( opacity < 1 )
		{
			/*calcular nova direcao do raio*/
			Vector normal =  objNormalAt( object, point );
			Vector innerRay = algSnell( ray, normal, 1.00029 , refractedIndex );
			Vector exitPoint = objInterceptExit( object, point, innerRay );
			normal = algUnit (objNormalAt( object, exitPoint ));
			ray = algSnell( innerRay, algScale( -1, normal ), refractedIndex, 1.00029 );

			colorOpacity = rayTrace( scene, exitPoint, ray, depth );							
			factor = 1 - opacity ;// fator para cor propria
			intensitycolor = colorScale( factor , colorOpacity);//contribuicao de cor de tr�s
			colorAux = colorScale( opacity , color );//contribuicao da propria cor
			color = colorAddition ( intensitycolor, colorAux );
		}
		
		/* Verifica Opacidade */
		InterceptionOpacity = isInShadow(scene, point, pointToLightRay, softlight);
			
		if( opacity > 0 )
		{
			/* Luz Difusa */
			produto = algDot( normal, pointToLightRay );
			if (produto > 0)
			{
				intensitycolor = colorScale( produto, diffuse );
				intensitycolor = colorScale( opacity, intensitycolor );
					if( InterceptionOpacity != 0 )
					{
						intensitycolor = colorScale( 1 - InterceptionOpacity , intensitycolor );
					}
					color = colorAddition(color, intensitycolor);
			}
		}

		/* Reflexao Especular */
		pointToLightRayReflected = algReflect( pointToLightRay , normal );// refletir vetor em torno da normal	
		produto = algDot( pointToLightRayReflected, pointToEyeRay );
		
		if ( produto > 0 )
			{ 
				produto = pow ( algDot( pointToLightRayReflected, pointToEyeRay ) , specularExponent );
				intensitycolor = colorScale( produto, specular );
					if( InterceptionOpacity != 0 )
					{
						intensitycolor = colorScale( 1 - InterceptionOpacity , intensitycolor );
					}
					color = colorAddition( color, intensitycolor );
			}
			

		/* Aplicando Efeito de Espelho */
		if (reflectionFactor > 0)
		{
			reflecRay = algReflect( pointToEyeRay, normal );		  
			colorReflec = rayTrace( scene, point ,reflecRay , depth + 1);
			intensitycolor = colorScale( reflectionFactor , colorReflec);//contribuicao de cor refletida
			color = colorAddition ( color, intensitycolor );
		}
	}

	return color;
}

static double getNearestObject( Scene* scene, Vector eye, Vector ray, Object** object )
{
	int i;
	int objectCount = sceneGetObjectCount( scene );

	double closest = DBL_MAX;

	/* Para cada objeto na cena */
	for( i = 0; i < objectCount; ++i ) {
		Object* currentObject = sceneGetObject( scene, i );
		double distance = objIntercept( currentObject, eye, ray );
		
		if( distance > 0.00001 && distance < closest )
		{
			closest = distance;
			*object = currentObject;
		}
	}

	return closest;
}

static double isInShadow( Scene* scene, Vector point, Vector rayToLight, Vector lightLocation )
{
	int i;
	int objectCount = sceneGetObjectCount( scene );
	double opacity = 0;

	/* maxDistance = dist�ncia de point at� lightLocation */
	double maxDistance = algNorm( algSub( lightLocation, point ) );

	/* Para cada objeto na cena */
	for( i = 0; i < objectCount; ++i )
	{
		double distance = objIntercept( sceneGetObject( scene, i ), point, rayToLight );
		
		if( distance > 0.1 && distance < maxDistance )
		{
			Object* interceptionObject = sceneGetObject( scene, i );
			Material* material = sceneGetMaterial(scene,objGetMaterial(interceptionObject));
			opacity += materialGetOpacity( material );
		}
	}

	return opacity;
}

