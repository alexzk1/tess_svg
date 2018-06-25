package biz.an_droid.beammer.models;

import com.badlogic.gdx.math.Polygon;
import com.badlogic.gdx.math.Rectangle;

/**
 * Created by alex (alexzkhr@gmail.com) on 7/4/15.
 * At 19:03
 */

/**
 * class ModelPolygon is same as com.badlogic.gdx.math.Polygon
 * except it allows to use "builder-like" chained notatation (setOrigion(0,0).translate(....)
 * and has extra method translateModel, which allows to move/rotate base model
 *
 */
public class ModelPolygon
{
    private Polygon polygon;

    public ModelPolygon(float[] vertices)
    {
        polygon = new Polygon(vertices);
    }

    public ModelPolygon copy()
    {
        ModelPolygon c = new ModelPolygon(polygon.getVertices());
        c.setOrigin(getOriginX(), getOriginY());
        c.setPosition(getX(), getY());
        c.setRotation(getRotation());
        c.setScale(getScaleX(), getScaleY());
        return c;
    }

    public Polygon get()
    {
        return polygon;
    }

    public float[] getVertices()
    {
        return polygon.getVertices();
    }

    public float[] getTransformedVertices()
    {
        return polygon.getTransformedVertices();
    }

    /**
     * Sets the point where all vertexes are relative to, this will be rotation center as well
     * @param originX
     * @param originY
     * @return
     */
    public ModelPolygon setOrigin(float originX, float originY)
    {
        polygon.setOrigin(originX, originY);
        return this;
    }

    public ModelPolygon setOrigin(float[] arr)
    {
        return setOrigin(arr[0], arr[1]);
    }

    /**
     * Sets the world position
     * @param x
     * @param y
     * @return
     */
    public ModelPolygon setPosition(float x, float y)
    {
        polygon.setPosition(x, y);
        return this;
    }

    public ModelPolygon setVertices(float[] vertices)
    {
        polygon.setVertices(vertices);
        return this;
    }

    public ModelPolygon translate(float x, float y)
    {
        polygon.translate(x, y);
        return this;
    }

    public ModelPolygon translateModel(float x, float y, float deg)
    {
        float[] v = getVertices();
        Polygon p = new Polygon(v);
        p.setRotation(deg);
        p.translate(x,y);
        polygon.setVertices(p.getTransformedVertices());
        setOrigin(getOriginX() + x, getOriginY() + y);
        return this;
    }

    public ModelPolygon setRotation(float degrees)
    {
        polygon.setRotation(degrees);
        return this;
    }

    public ModelPolygon rotate(float degrees)
    {
        polygon.rotate(degrees);
        return this;
    }

    public ModelPolygon setScale(float scaleX, float scaleY)
    {
        polygon.setScale(scaleX, scaleY);
        return this;
    }

    public ModelPolygon scale(float amount)
    {
        polygon.scale(amount);
        return this;
    }

    public void dirty()
    {
        polygon.dirty();
    }

    public float area()
    {
        return polygon.area();
    }

    public Rectangle getBoundingRectangle()
    {
        return polygon.getBoundingRectangle();
    }

    public boolean contains(float x, float y)
    {
        return polygon.contains(x, y);
    }

    public float getX()
    {
        return polygon.getX();
    }

    public float getY()
    {
        return polygon.getY();
    }

    public float getOriginX()
    {
        return polygon.getOriginX();
    }

    public float getOriginY()
    {
        return polygon.getOriginY();
    }

    public float getRotation()
    {
        return polygon.getRotation();
    }

    public float getScaleX()
    {
        return polygon.getScaleX();
    }

    public float getScaleY()
    {
        return polygon.getScaleY();
    }
}
