















package com.nineoldandroids.animation;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

import android.view.animation.Interpolator;




















public final class AnimatorSet extends Animator {

    






    



    private ArrayList<Animator> mPlayingSet = new ArrayList<Animator>();

    





    private HashMap<Animator, Node> mNodeMap = new HashMap<Animator, Node>();

    




    private ArrayList<Node> mNodes = new ArrayList<Node>();

    




    private ArrayList<Node> mSortedNodes = new ArrayList<Node>();

    




    private boolean mNeedsSort = true;

    private AnimatorSetListener mSetListener = null;

    






    boolean mTerminated = false;

    



    private boolean mStarted = false;

    
    private long mStartDelay = 0;

    
    private ValueAnimator mDelayAnim = null;


    
    
    
    private long mDuration = -1;


    




    public void playTogether(Animator... items) {
        if (items != null) {
            mNeedsSort = true;
            Builder builder = play(items[0]);
            for (int i = 1; i < items.length; ++i) {
                builder.with(items[i]);
            }
        }
    }

    




    public void playTogether(Collection<Animator> items) {
        if (items != null && items.size() > 0) {
            mNeedsSort = true;
            Builder builder = null;
            for (Animator anim : items) {
                if (builder == null) {
                    builder = play(anim);
                } else {
                    builder.with(anim);
                }
            }
        }
    }

    





    public void playSequentially(Animator... items) {
        if (items != null) {
            mNeedsSort = true;
            if (items.length == 1) {
                play(items[0]);
            } else {
                for (int i = 0; i < items.length - 1; ++i) {
                    play(items[i]).before(items[i+1]);
                }
            }
        }
    }

    





    public void playSequentially(List<Animator> items) {
        if (items != null && items.size() > 0) {
            mNeedsSort = true;
            if (items.size() == 1) {
                play(items.get(0));
            } else {
                for (int i = 0; i < items.size() - 1; ++i) {
                    play(items.get(i)).before(items.get(i+1));
                }
            }
        }
    }

    







    public ArrayList<Animator> getChildAnimations() {
        ArrayList<Animator> childList = new ArrayList<Animator>();
        for (Node node : mNodes) {
            childList.add(node.animation);
        }
        return childList;
    }

    






    @Override
    public void setTarget(Object target) {
        for (Node node : mNodes) {
            Animator animation = node.animation;
            if (animation instanceof AnimatorSet) {
                ((AnimatorSet)animation).setTarget(target);
            } else if (animation instanceof ObjectAnimator) {
                ((ObjectAnimator)animation).setTarget(target);
            }
        }
    }

    





    @Override
    public void setInterpolator(Interpolator interpolator) {
        for (Node node : mNodes) {
            node.animation.setInterpolator(interpolator);
        }
    }

    



























    public Builder play(Animator anim) {
        if (anim != null) {
            mNeedsSort = true;
            return new Builder(anim);
        }
        return null;
    }

    





    @SuppressWarnings("unchecked")
    @Override
    public void cancel() {
        mTerminated = true;
        if (isStarted()) {
            ArrayList<AnimatorListener> tmpListeners = null;
            if (mListeners != null) {
                tmpListeners = (ArrayList<AnimatorListener>) mListeners.clone();
                for (AnimatorListener listener : tmpListeners) {
                    listener.onAnimationCancel(this);
                }
            }
            if (mDelayAnim != null && mDelayAnim.isRunning()) {
                
                
                mDelayAnim.cancel();
            } else  if (mSortedNodes.size() > 0) {
                for (Node node : mSortedNodes) {
                    node.animation.cancel();
                }
            }
            if (tmpListeners != null) {
                for (AnimatorListener listener : tmpListeners) {
                    listener.onAnimationEnd(this);
                }
            }
            mStarted = false;
        }
    }

    





    @Override
    public void end() {
        mTerminated = true;
        if (isStarted()) {
            if (mSortedNodes.size() != mNodes.size()) {
                
                sortNodes();
                for (Node node : mSortedNodes) {
                    if (mSetListener == null) {
                        mSetListener = new AnimatorSetListener(this);
                    }
                    node.animation.addListener(mSetListener);
                }
            }
            if (mDelayAnim != null) {
                mDelayAnim.cancel();
            }
            if (mSortedNodes.size() > 0) {
                for (Node node : mSortedNodes) {
                    node.animation.end();
                }
            }
            if (mListeners != null) {
                ArrayList<AnimatorListener> tmpListeners =
                        (ArrayList<AnimatorListener>) mListeners.clone();
                for (AnimatorListener listener : tmpListeners) {
                    listener.onAnimationEnd(this);
                }
            }
            mStarted = false;
        }
    }

    




    @Override
    public boolean isRunning() {
        for (Node node : mNodes) {
            if (node.animation.isRunning()) {
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean isStarted() {
        return mStarted;
    }

    





    @Override
    public long getStartDelay() {
        return mStartDelay;
    }

    





    @Override
    public void setStartDelay(long startDelay) {
        mStartDelay = startDelay;
    }

    







    @Override
    public long getDuration() {
        return mDuration;
    }

    







    @Override
    public AnimatorSet setDuration(long duration) {
        if (duration < 0) {
            throw new IllegalArgumentException("duration must be a value of zero or greater");
        }
        for (Node node : mNodes) {
            
            
            node.animation.setDuration(duration);
        }
        mDuration = duration;
        return this;
    }

    @Override
    public void setupStartValues() {
        for (Node node : mNodes) {
            node.animation.setupStartValues();
        }
    }

    @Override
    public void setupEndValues() {
        for (Node node : mNodes) {
            node.animation.setupEndValues();
        }
    }

    






    @SuppressWarnings("unchecked")
    @Override
    public void start() {
        mTerminated = false;
        mStarted = true;

        
        
        sortNodes();

        int numSortedNodes = mSortedNodes.size();
        for (int i = 0; i < numSortedNodes; ++i) {
            Node node = mSortedNodes.get(i);
            
            ArrayList<AnimatorListener> oldListeners = node.animation.getListeners();
            if (oldListeners != null && oldListeners.size() > 0) {
                final ArrayList<AnimatorListener> clonedListeners = new
                        ArrayList<AnimatorListener>(oldListeners);

                for (AnimatorListener listener : clonedListeners) {
                    if (listener instanceof DependencyListener ||
                            listener instanceof AnimatorSetListener) {
                        node.animation.removeListener(listener);
                    }
                }
            }
        }

        
        
        
        
        final ArrayList<Node> nodesToStart = new ArrayList<Node>();
        for (int i = 0; i < numSortedNodes; ++i) {
            Node node = mSortedNodes.get(i);
            if (mSetListener == null) {
                mSetListener = new AnimatorSetListener(this);
            }
            if (node.dependencies == null || node.dependencies.size() == 0) {
                nodesToStart.add(node);
            } else {
                int numDependencies = node.dependencies.size();
                for (int j = 0; j < numDependencies; ++j) {
                    Dependency dependency = node.dependencies.get(j);
                    dependency.node.animation.addListener(
                            new DependencyListener(this, node, dependency.rule));
                }
                node.tmpDependencies = (ArrayList<Dependency>) node.dependencies.clone();
            }
            node.animation.addListener(mSetListener);
        }
        
        if (mStartDelay <= 0) {
            for (Node node : nodesToStart) {
                node.animation.start();
                mPlayingSet.add(node.animation);
            }
        } else {
            mDelayAnim = ValueAnimator.ofFloat(0f, 1f);
            mDelayAnim.setDuration(mStartDelay);
            mDelayAnim.addListener(new AnimatorListenerAdapter() {
                boolean canceled = false;
                public void onAnimationCancel(Animator anim) {
                    canceled = true;
                }
                public void onAnimationEnd(Animator anim) {
                    if (!canceled) {
                        int numNodes = nodesToStart.size();
                        for (int i = 0; i < numNodes; ++i) {
                            Node node = nodesToStart.get(i);
                            node.animation.start();
                            mPlayingSet.add(node.animation);
                        }
                    }
                }
            });
            mDelayAnim.start();
        }
        if (mListeners != null) {
            ArrayList<AnimatorListener> tmpListeners =
                    (ArrayList<AnimatorListener>) mListeners.clone();
            int numListeners = tmpListeners.size();
            for (int i = 0; i < numListeners; ++i) {
                tmpListeners.get(i).onAnimationStart(this);
            }
        }
        if (mNodes.size() == 0 && mStartDelay == 0) {
            
            
            mStarted = false;
            if (mListeners != null) {
                ArrayList<AnimatorListener> tmpListeners =
                        (ArrayList<AnimatorListener>) mListeners.clone();
                int numListeners = tmpListeners.size();
                for (int i = 0; i < numListeners; ++i) {
                    tmpListeners.get(i).onAnimationEnd(this);
                }
            }
        }
    }

    @Override
    public AnimatorSet clone() {
        final AnimatorSet anim = (AnimatorSet) super.clone();
        







        anim.mNeedsSort = true;
        anim.mTerminated = false;
        anim.mStarted = false;
        anim.mPlayingSet = new ArrayList<Animator>();
        anim.mNodeMap = new HashMap<Animator, Node>();
        anim.mNodes = new ArrayList<Node>();
        anim.mSortedNodes = new ArrayList<Node>();

        
        
        
        HashMap<Node, Node> nodeCloneMap = new HashMap<Node, Node>(); 
        for (Node node : mNodes) {
            Node nodeClone = node.clone();
            nodeCloneMap.put(node, nodeClone);
            anim.mNodes.add(nodeClone);
            anim.mNodeMap.put(nodeClone.animation, nodeClone);
            
            nodeClone.dependencies = null;
            nodeClone.tmpDependencies = null;
            nodeClone.nodeDependents = null;
            nodeClone.nodeDependencies = null;
            
            
            ArrayList<AnimatorListener> cloneListeners = nodeClone.animation.getListeners();
            if (cloneListeners != null) {
                ArrayList<AnimatorListener> listenersToRemove = null;
                for (AnimatorListener listener : cloneListeners) {
                    if (listener instanceof AnimatorSetListener) {
                        if (listenersToRemove == null) {
                            listenersToRemove = new ArrayList<AnimatorListener>();
                        }
                        listenersToRemove.add(listener);
                    }
                }
                if (listenersToRemove != null) {
                    for (AnimatorListener listener : listenersToRemove) {
                        cloneListeners.remove(listener);
                    }
                }
            }
        }
        
        
        for (Node node : mNodes) {
            Node nodeClone = nodeCloneMap.get(node);
            if (node.dependencies != null) {
                for (Dependency dependency : node.dependencies) {
                    Node clonedDependencyNode = nodeCloneMap.get(dependency.node);
                    Dependency cloneDependency = new Dependency(clonedDependencyNode,
                            dependency.rule);
                    nodeClone.addDependency(cloneDependency);
                }
            }
        }

        return anim;
    }

    




    private static class DependencyListener implements AnimatorListener {

        private AnimatorSet mAnimatorSet;

        
        private Node mNode;

        
        
        private int mRule;

        public DependencyListener(AnimatorSet animatorSet, Node node, int rule) {
            this.mAnimatorSet = animatorSet;
            this.mNode = node;
            this.mRule = rule;
        }

        




        public void onAnimationCancel(Animator animation) {
        }

        


        public void onAnimationEnd(Animator animation) {
            if (mRule == Dependency.AFTER) {
                startIfReady(animation);
            }
        }

        


        public void onAnimationRepeat(Animator animation) {
        }

        


        public void onAnimationStart(Animator animation) {
            if (mRule == Dependency.WITH) {
                startIfReady(animation);
            }
        }

        





        private void startIfReady(Animator dependencyAnimation) {
            if (mAnimatorSet.mTerminated) {
                
                return;
            }
            Dependency dependencyToRemove = null;
            int numDependencies = mNode.tmpDependencies.size();
            for (int i = 0; i < numDependencies; ++i) {
                Dependency dependency = mNode.tmpDependencies.get(i);
                if (dependency.rule == mRule &&
                        dependency.node.animation == dependencyAnimation) {
                    
                    
                    dependencyToRemove = dependency;
                    dependencyAnimation.removeListener(this);
                    break;
                }
            }
            mNode.tmpDependencies.remove(dependencyToRemove);
            if (mNode.tmpDependencies.size() == 0) {
                
                mNode.animation.start();
                mAnimatorSet.mPlayingSet.add(mNode.animation);
            }
        }

    }

    private class AnimatorSetListener implements AnimatorListener {

        private AnimatorSet mAnimatorSet;

        AnimatorSetListener(AnimatorSet animatorSet) {
            mAnimatorSet = animatorSet;
        }

        public void onAnimationCancel(Animator animation) {
            if (!mTerminated) {
                
                
                if (mPlayingSet.size() == 0) {
                    if (mListeners != null) {
                        int numListeners = mListeners.size();
                        for (int i = 0; i < numListeners; ++i) {
                            mListeners.get(i).onAnimationCancel(mAnimatorSet);
                        }
                    }
                }
            }
        }

        @SuppressWarnings("unchecked")
        public void onAnimationEnd(Animator animation) {
            animation.removeListener(this);
            mPlayingSet.remove(animation);
            Node animNode = mAnimatorSet.mNodeMap.get(animation);
            animNode.done = true;
            if (!mTerminated) {
                
                
                ArrayList<Node> sortedNodes = mAnimatorSet.mSortedNodes;
                boolean allDone = true;
                int numSortedNodes = sortedNodes.size();
                for (int i = 0; i < numSortedNodes; ++i) {
                    if (!sortedNodes.get(i).done) {
                        allDone = false;
                        break;
                    }
                }
                if (allDone) {
                    
                    
                    if (mListeners != null) {
                        ArrayList<AnimatorListener> tmpListeners =
                                (ArrayList<AnimatorListener>) mListeners.clone();
                        int numListeners = tmpListeners.size();
                        for (int i = 0; i < numListeners; ++i) {
                            tmpListeners.get(i).onAnimationEnd(mAnimatorSet);
                        }
                    }
                    mAnimatorSet.mStarted = false;
                }
            }
        }

        
        public void onAnimationRepeat(Animator animation) {
        }

        
        public void onAnimationStart(Animator animation) {
        }

    }

    









    private void sortNodes() {
        if (mNeedsSort) {
            mSortedNodes.clear();
            ArrayList<Node> roots = new ArrayList<Node>();
            int numNodes = mNodes.size();
            for (int i = 0; i < numNodes; ++i) {
                Node node = mNodes.get(i);
                if (node.dependencies == null || node.dependencies.size() == 0) {
                    roots.add(node);
                }
            }
            ArrayList<Node> tmpRoots = new ArrayList<Node>();
            while (roots.size() > 0) {
                int numRoots = roots.size();
                for (int i = 0; i < numRoots; ++i) {
                    Node root = roots.get(i);
                    mSortedNodes.add(root);
                    if (root.nodeDependents != null) {
                        int numDependents = root.nodeDependents.size();
                        for (int j = 0; j < numDependents; ++j) {
                            Node node = root.nodeDependents.get(j);
                            node.nodeDependencies.remove(root);
                            if (node.nodeDependencies.size() == 0) {
                                tmpRoots.add(node);
                            }
                        }
                    }
                }
                roots.clear();
                roots.addAll(tmpRoots);
                tmpRoots.clear();
            }
            mNeedsSort = false;
            if (mSortedNodes.size() != mNodes.size()) {
                throw new IllegalStateException("Circular dependencies cannot exist"
                        + " in AnimatorSet");
            }
        } else {
            
            
            
            int numNodes = mNodes.size();
            for (int i = 0; i < numNodes; ++i) {
                Node node = mNodes.get(i);
                if (node.dependencies != null && node.dependencies.size() > 0) {
                    int numDependencies = node.dependencies.size();
                    for (int j = 0; j < numDependencies; ++j) {
                        Dependency dependency = node.dependencies.get(j);
                        if (node.nodeDependencies == null) {
                            node.nodeDependencies = new ArrayList<Node>();
                        }
                        if (!node.nodeDependencies.contains(dependency.node)) {
                            node.nodeDependencies.add(dependency.node);
                        }
                    }
                }
                
                
                node.done = false;
            }
        }
    }

    




    private static class Dependency {
        static final int WITH = 0; 
        static final int AFTER = 1; 

        
        public Node node;

        
        public int rule;

        public Dependency(Node node, int rule) {
            this.node = node;
            this.rule = rule;
        }
    }

    





    private static class Node implements Cloneable {
        public Animator animation;

        





        public ArrayList<Dependency> dependencies = null;

        








        public ArrayList<Dependency> tmpDependencies = null;

        



        public ArrayList<Node> nodeDependencies = null;

        




        public ArrayList<Node> nodeDependents = null;

        




        public boolean done = false;

        






        public Node(Animator animation) {
            this.animation = animation;
        }

        




        public void addDependency(Dependency dependency) {
            if (dependencies == null) {
                dependencies = new ArrayList<Dependency>();
                nodeDependencies = new ArrayList<Node>();
            }
            dependencies.add(dependency);
            if (!nodeDependencies.contains(dependency.node)) {
                nodeDependencies.add(dependency.node);
            }
            Node dependencyNode = dependency.node;
            if (dependencyNode.nodeDependents == null) {
                dependencyNode.nodeDependents = new ArrayList<Node>();
            }
            dependencyNode.nodeDependents.add(this);
        }

        @Override
        public Node clone() {
            try {
                Node node = (Node) super.clone();
                node.animation = (Animator) animation.clone();
                return node;
            } catch (CloneNotSupportedException e) {
               throw new AssertionError();
            }
        }
    }

    



















































    public class Builder {

        



        private Node mCurrentNode;

        






        Builder(Animator anim) {
            mCurrentNode = mNodeMap.get(anim);
            if (mCurrentNode == null) {
                mCurrentNode = new Node(anim);
                mNodeMap.put(anim, mCurrentNode);
                mNodes.add(mCurrentNode);
            }
        }

        






        public Builder with(Animator anim) {
            Node node = mNodeMap.get(anim);
            if (node == null) {
                node = new Node(anim);
                mNodeMap.put(anim, node);
                mNodes.add(node);
            }
            Dependency dependency = new Dependency(mCurrentNode, Dependency.WITH);
            node.addDependency(dependency);
            return this;
        }

        







        public Builder before(Animator anim) {
            Node node = mNodeMap.get(anim);
            if (node == null) {
                node = new Node(anim);
                mNodeMap.put(anim, node);
                mNodes.add(node);
            }
            Dependency dependency = new Dependency(mCurrentNode, Dependency.AFTER);
            node.addDependency(dependency);
            return this;
        }

        







        public Builder after(Animator anim) {
            Node node = mNodeMap.get(anim);
            if (node == null) {
                node = new Node(anim);
                mNodeMap.put(anim, node);
                mNodes.add(node);
            }
            Dependency dependency = new Dependency(node, Dependency.AFTER);
            mCurrentNode.addDependency(dependency);
            return this;
        }

        







        public Builder after(long delay) {
            
            ValueAnimator anim = ValueAnimator.ofFloat(0f, 1f);
            anim.setDuration(delay);
            after(anim);
            return this;
        }

    }

}
